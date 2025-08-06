#include "viewrenderer.h"
#include "view.h"

#include "graphics/material.h"
#include "graphics/mesh.h"
#include "render/context.h"
#include "render/device.h"

static constexpr vkr::Vector2f JitterHaltonSequence[] = { 
	{0.5,0.333333},
	{0.25,0.666667},
	{0.750000, 0.111111},
	{0.125000, 0.444444},
	{0.625000, 0.777778},
	{0.375000, 0.222222},
	{0.875000, 0.555556},
	{0.062500, 0.888889},
	{0.562500, 0.037037},
	{0.312500, 0.370370},
	{0.812500, 0.703704},
	{0.187500, 0.148148},
	{0.687500, 0.481481},
	{0.437500, 0.814815},
	{0.937500, 0.259259},
	{ 0.031250, 0.592593 }
};

namespace vkr::Graphics
{
	ViewRenderer::ViewRenderer()
	{

	}

	ViewRenderer::~ViewRenderer()
	{

	}

	bool ViewRenderer::Init()
	{
		Render::Device* device = Render::GetDevice();
		// init any renderer subsystems
		// ex. upscalers, water, vegetation, environment, particle/vfx, light culling

		//Initializing global shaders/resources

		// Static object velocity computing
		m_StaticVelShader = device->CreateShader("../../../content/shaders/staticvel.hlsl", L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc staticVelPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		staticVelPSODesc.Compute.m_ComputeShader = m_StaticVelShader.get();
		m_StaticVelPSO = device->CreatePipelineState(staticVelPSODesc);

		//Sky
		
		m_SkyComputeShader = device->CreateShader("../../../content/shaders/sky.hlsl", L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyPSODesc.Compute.m_ComputeShader = m_SkyComputeShader.get();
		m_SkyPSO = device->CreatePipelineState(skyPSODesc);

		//TAA
				
		m_TAAResolveComputeShader = device->CreateShader("../../../content/shaders/taa.hlsl", L"ResolveCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc taaPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		taaPSODesc.Compute.m_ComputeShader = m_TAAResolveComputeShader.get();
		m_TAAResolvePSO = device->CreatePipelineState(taaPSODesc);

		// Raytrace
		m_RaytraceShader = device->CreateShader("../../../content/shaders/tracerays.hlsl", L"Main", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc raytracePSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		raytracePSODesc.Compute.m_ComputeShader = m_RaytraceShader.get();
		m_RaytracePSO = device->CreatePipelineState(raytracePSODesc);

		return true;
	}

	void ViewRenderer::RenderView(View& view)
	{
		View* viewPtr = &view; 
		viewPtr->BeginRender();
		Render::QueueGraphicsTask([this, viewPtr]() mutable { PreRenderUpdates(*viewPtr); });
		//UpdateParticles(view);
		Render::QueueGraphicsTask([this, viewPtr]() mutable { DepthPrepass(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { StaticVelocity(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { TraceRadiance(*viewPtr); });
// 		Render::QueueGraphicsTask([this, viewPtr]() mutable { ForwardPass(*viewPtr); });
// 		Render::QueueGraphicsTask([this, viewPtr]() mutable { RenderSky(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { ApplyUpscaling(*viewPtr); });
		//ApplyPostEffects(view);
		Ref<Render::RenderTaskEvent> lastRenderEvent = Render::QueueGraphicsTask([this, viewPtr]() mutable { FinalizeFrame(*viewPtr); });
		viewPtr->EndRender();
	}

	void ViewRenderer::PreRenderUpdates(View& view)
	{
		ViewRenderData& renderData = view.GetMutableRenderData();

		renderData.m_MaterialDataBuffer.PrepareBuffer();

		Ref<Render::Buffer> rtTLAS = Render::GetDevice()->CreateTLAS(renderData.m_RaytracingInstances.size(), renderData.m_RaytracingInstances.data());

		Render::BufferViewDesc rtTLASDesc = {};
		rtTLASDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAYTRACING_ACCELERATION_STRUCTURE;
		renderData.m_RaytracingTLAS = Render::GetDevice()->CreateBufferView(rtTLASDesc, rtTLAS);

		//Fill in the instance data (later we can just keep this as a normal buffer instead of temp that we need to rebuild per frame)
		auto instanceDataBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, renderData.m_InstanceData.size(), renderData.m_InstanceData.size(), renderData.m_InstanceData.data());

		Render::BufferViewDesc instanceDataBufferDesc = {}; // Byteaddressbuffer
		instanceDataBufferDesc.m_ElementStart = instanceDataBuffer.m_Offset;
		instanceDataBufferDesc.m_ElementCount = renderData.m_InstanceData.size();
		instanceDataBufferDesc.m_ElementSize = 1;
		instanceDataBufferDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAW;
		instanceDataBufferDesc.m_Format = Render::FORMAT_UNKNOWN;
		renderData.m_InstanceDataBufferView = Render::GetDevice()->CreateBufferView(instanceDataBufferDesc, instanceDataBuffer.m_Buffer);

		//Fill in the instance data indices
		auto instanceDataOffsetBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), renderData.m_InstanceDataOffsetBuffer.data());

		Render::BufferViewDesc instanceDataOffsetBufferDesc = {}; // Typed uint buffer
		instanceDataOffsetBufferDesc.m_ElementStart = instanceDataOffsetBuffer.m_Offset / sizeof(uint32_t);
		instanceDataOffsetBufferDesc.m_ElementCount = renderData.m_InstanceDataOffsetBuffer.size();
		instanceDataOffsetBufferDesc.m_Usage = Render::BUFFER_VIEW_USAGE_TYPED;
		instanceDataOffsetBufferDesc.m_Format = Render::FORMAT_R32_UINT;
		renderData.m_InstanceDataOffsetBufferView = Render::GetDevice()->CreateBufferView(instanceDataOffsetBufferDesc, instanceDataBuffer.m_Buffer);

		//Construct the per scene constant buffer
		struct alignas(16) PerSceneConstantData
		{
			Mat44 View;
			Mat44 InvView;
			Mat44 Projection;
			Mat44 InvProjection;
			Mat44 ViewProjection;
			Mat44 InvViewProjection;
			Mat44 PrevViewProjection;
			Vector2f CurrentJitter;
			Vector2f PrevJitter;
			uint32_t FrameIndex;
			float DeltaTime;
			float ElapsedTime;
			uint32_t _pad;
			uint32_t InstanceDataBufferDescriptorIndex; // Descriptor index to the global buffer where all instance data for the scene is stored
			uint32_t InstanceDataOffsetBufferDescriptorIndex;
			uint32_t MaterialDataBufferDescriptorIndex;
			uint32_t RaytracingSceneDescriptorIndex;
			Vector3f CameraWorldPosition;
			uint32_t NumDirectionalLightsInUse;
			DirectionalLight DirectionalLights[2];
		};

		PerSceneConstantData perSceneConstantData = {};

		Mat44 ProjectionNoJitter = const_cast<Camera&>(view.GetCamera()).GetProjection();
		//Select a new jitter offset for TAA for this frame
		int jitterIdx = m_CurrentJitterIndex++;
		m_CurrentJitterIndex = m_CurrentJitterIndex % 16;
		perSceneConstantData.CurrentJitter = (JitterHaltonSequence[jitterIdx] - 0.5f) / Vector2f(view.GetRenderSize()) * 2.0f;
		Mat44 Projection = ProjectionNoJitter;
		Projection[8] = perSceneConstantData.CurrentJitter.x;
		Projection[9] = perSceneConstantData.CurrentJitter.y;
		perSceneConstantData.PrevJitter = m_PrevJitter;
		m_PrevJitter = perSceneConstantData.CurrentJitter;
		perSceneConstantData.FrameIndex = ElapsedTimer::FrameIndex();
		perSceneConstantData.DeltaTime = ElapsedTimer::DeltaTime();
		perSceneConstantData.ElapsedTime = ElapsedTimer::ElapsedTime();

		Mat43 CamWorld = const_cast<Camera&>(view.GetCamera()).GetWorldTransform();
		perSceneConstantData.CameraWorldPosition = Vector3f(CamWorld[9], CamWorld[10], CamWorld[11]);
		perSceneConstantData.View = const_cast<Camera&>(view.GetCamera()).GetView();
		perSceneConstantData.InvView = Inverse(perSceneConstantData.View);
		perSceneConstantData.Projection = Projection;
		perSceneConstantData.InvProjection = Inverse(perSceneConstantData.Projection);
		perSceneConstantData.ViewProjection = perSceneConstantData.View * perSceneConstantData.Projection;
		perSceneConstantData.InvViewProjection = Inverse(perSceneConstantData.ViewProjection);
		perSceneConstantData.PrevViewProjection = m_PrevViewProjection;
		m_PrevViewProjection = perSceneConstantData.ViewProjection;
		perSceneConstantData.InstanceDataBufferDescriptorIndex = renderData.m_InstanceDataBufferView->GetIndex();
		perSceneConstantData.InstanceDataOffsetBufferDescriptorIndex = renderData.m_InstanceDataOffsetBufferView->GetIndex();
		perSceneConstantData.MaterialDataBufferDescriptorIndex = renderData.m_MaterialDataBuffer.GetBufferView()->GetIndex();
		perSceneConstantData.RaytracingSceneDescriptorIndex = renderData.m_RaytracingTLAS->GetIndex();

		perSceneConstantData.NumDirectionalLightsInUse = 1;

		perSceneConstantData.DirectionalLights[0].Emission = Vector3f(6.0, 6.0, 6.0);
		perSceneConstantData.DirectionalLights[0].Direction = Vector3f(0.4, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[0].Radius = tanf(DegToRad(0.53f));

		perSceneConstantData.DirectionalLights[1].Emission = Vector3f(8.0, 2.0, 2.0);
		perSceneConstantData.DirectionalLights[1].Direction = Vector3f(-0.4, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[1].Radius = 0.02f;

		renderData.m_PerSceneConstantBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(PerSceneConstantData), sizeof(PerSceneConstantData), &perSceneConstantData);

		Render::Context* ctx = Render::Context::GetCurrentContext();
		ctx->ClearStateCache();
		ctx->BindGlobalConstantBuffer(renderData.m_PerSceneConstantBuffer.m_Buffer.get(), renderData.m_PerSceneConstantBuffer.m_Offset, Render::GLOBAL_CONSTANT_BUFFER_SCENE);

	}

	void ViewRenderer::UpdateParticles(View& view)
	{

	}

	void ViewRenderer::DepthPrepass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();

		renderTargets.m_DepthBuffer.m_IsDepthStencil = true;
		renderTargets.m_DepthBuffer.m_Format = Render::Format::FORMAT_D32_FLOAT;
		renderTargets.m_DepthBuffer.Update(view.GetRenderSize(), "ViewRenderTargets::DepthBuffer");

		ctx->InsertWait(renderData.m_RaytracingTLAS->GetBuffer()->GetGpuPending());
		SET_CONTEXT_MARKER_FUNCTION(ctx);
		//Depth prepass Transitions
		{
			//Transition DS to write
			std::vector<Render::TextureBarrierDesc> barriers;
			{
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_DepthBuffer.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_DEPTH_STENCIL;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_WRITE;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL_WRITE;
				barriers.push_back(barrierDesc);
			}
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		ctx->ClearDepthStencil(renderTargets.m_DepthBuffer.m_DepthStencil.get(), 0.0f);

		std::vector<Render::RenderTargetView*> targets;
		ctx->BindRenderTargets(targets.size(), targets.data());
		ctx->BindDepthStencil(renderTargets.m_DepthBuffer.m_DepthStencil.get());

		const Render::TextureDesc& rtDesc = renderTargets.m_DepthBuffer.m_Texture->m_TextureDesc;
		ctx->SetViewport(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		ctx->SetScissorRect(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		
		for (auto& batch : renderData.m_DepthPassData.m_InstanceBatches)
		{
			ctx->BindVertexBuffer(batch.m_Mesh->GetVertexBuffer().get());
			ctx->BindIndexBuffer(batch.m_Mesh->GetIndexBuffer().get());
			ctx->SetPrimitiveTopology(batch.m_Mesh->GetTopology());
			ctx->BindPipelineState(batch.m_PSO.get());

			struct alignas(16) ConstantData
			{
				uint32_t m_BatchStart;
				uint32_t RaytracingSceneDescriptor;
			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;
			data.RaytracingSceneDescriptor = renderData.m_RaytracingTLAS->GetIndex();
			ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

			ctx->DrawIndexedInstanced(batch.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount, batch.m_Count);
		}
		
	}

	void ViewRenderer::StaticVelocity(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();

		renderTargets.m_SceneBuffer_RenderSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_RenderSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_RenderSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_RenderSize.Update(view.GetRenderSize(), "ViewRenderTargets::SceneBuffer_RenderSize");

		renderTargets.m_Velocity.m_IsWritable = true;
		renderTargets.m_Velocity.m_IsRenderTarget = true;
		renderTargets.m_Velocity.m_Format = Render::Format::FORMAT_RG16_FLOAT;
		uint32_t nanBits = 0xffffffff;
		float nanValue = *reinterpret_cast<float*>(&nanBits);
		renderTargets.m_Velocity.m_ClearValue = { nanValue, nanValue, nanValue, nanValue };
		renderTargets.m_Velocity.Update(view.GetRenderSize(), "ViewRenderTargets::Velocity");

		SET_CONTEXT_MARKER_FUNCTION(ctx);

		// At this point dynamic objects should have velocity (computed in PS in depth prepass)
		// Add a small compute pass to add velocity to all static geometry (existing depth but no velocity written)
		// Static velocity computing transitions
		{
			//Transition DS to read
			std::vector<Render::TextureBarrierDesc> barriers;
			{
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_DepthBuffer.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_DEPTH_STENCIL;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_READ;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL_READ;
				barriers.push_back(barrierDesc);
			}
			//Transition the velocity buffer to render target for the clear
			{
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_Velocity.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
				barriers.push_back(barrierDesc);
			}
			//Transition the scene buffer to render target for the clear
			{
				Render::TextureBarrierDesc barrierDesc;
				barrierDesc.m_Texture = renderTargets.m_SceneBuffer_RenderSize.m_Texture.get();
				barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
				barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
				barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
				barriers.push_back(barrierDesc);
			}
			ctx->TextureBarrier(barriers.size(), barriers.data());
		}

		{
			Vector4f clearValues[2] = { {0,0,0,0},{nanValue,nanValue,nanValue,nanValue} };
			std::vector<Render::RenderTargetView*> targets;
			targets.push_back(renderTargets.m_SceneBuffer_RenderSize.m_RenderTarget.get());
			targets.push_back(renderTargets.m_Velocity.m_RenderTarget.get());
			ctx->ClearRenderTargets(targets.size(), targets.data(), clearValues);

			{
				std::vector<Render::TextureBarrierDesc> barriers;
				//Transition the velocity buffer to write
				{
					Render::TextureBarrierDesc barrierDesc;
					barrierDesc.m_Texture = renderTargets.m_Velocity.m_Texture.get();
					barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
					barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
					barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
					barriers.push_back(barrierDesc);
				}
				ctx->TextureBarrier(barriers.size(), barriers.data());
			}

			ctx->BindPipelineState(m_StaticVelPSO.get());

			struct alignas(16) ConstantData
			{
				uint32_t DepthBufferDescriptorIndex;
				uint32_t VelocityBufferDescriptorIndex;
			};
			ConstantData data;
			data.DepthBufferDescriptorIndex = view.GetRenderTargets().m_DepthBuffer.m_TextureView->GetIndex();
			data.VelocityBufferDescriptorIndex = view.GetRenderTargets().m_Velocity.m_TextureViewRW->GetIndex();

			ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

			ctx->DispatchThreads({ view.GetRenderSize().x, view.GetRenderSize().y, 1 });
		}
	}

	void ViewRenderer::ForwardPass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		renderTargets.m_SceneBuffer_RenderSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_RenderSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_RenderSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_RenderSize.Update(view.GetRenderSize(), "ViewRenderTargets::SceneBuffer_RenderSize");

		renderTargets.m_SceneBuffer_OutputSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_OutputSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_OutputSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_OutputSize.Update(view.GetRenderSize(), "ViewRenderTargets::SceneBuffer_OutputSize");

		renderTargets.m_Velocity.m_IsWritable = true;
		renderTargets.m_Velocity.m_IsRenderTarget = true;
		renderTargets.m_Velocity.m_Format = Render::Format::FORMAT_RG16_FLOAT;
		renderTargets.m_Velocity.m_ClearValue = { FLT_MAX,FLT_MAX,FLT_MAX ,FLT_MAX };
		renderTargets.m_Velocity.Update(view.GetRenderSize(), "ViewRenderTargets::Velocity");

		//Transition to RT the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneBuffer_RenderSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
			barriers.push_back(barrierDesc);
		}
		//Transition to RT the velocity buffer
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_Velocity.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
			barriers.push_back(barrierDesc);
		}
		//We will only read from DS now that depths are written to
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_DepthBuffer.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_READ;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL_READ;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());


		std::vector<Render::RenderTargetView*> targets;
		targets.push_back(renderTargets.m_SceneBuffer_RenderSize.m_RenderTarget.get());
		targets.push_back(renderTargets.m_Velocity.m_RenderTarget.get());

		ctx->ClearRenderTargets(targets.size(), targets.data());
		//ctx->ClearDepthStencil(view.GetDepthBuffer(), 0.0f);

		ctx->BindRenderTargets(targets.size(), targets.data());
		ctx->BindDepthStencil(renderTargets.m_DepthBuffer.m_DepthStencil.get());

		const Render::TextureDesc& rtDesc = view.GetOutputTarget()->GetTexture()->m_TextureDesc;
		ctx->SetViewport(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		ctx->SetScissorRect(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);

		for (auto& batch : renderData.m_ForwardPassData.m_InstanceBatches)
		{
			ctx->BindVertexBuffer(batch.m_Mesh->GetVertexBuffer().get());
			ctx->BindIndexBuffer(batch.m_Mesh->GetIndexBuffer().get());
			ctx->SetPrimitiveTopology(batch.m_Mesh->GetTopology());
			ctx->BindPipelineState(batch.m_PSO.get());

			struct alignas(16) ConstantData
			{
				uint32_t m_BatchStart;
				uint32_t RaytracingSceneDescriptor;
			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;
			data.RaytracingSceneDescriptor = renderData.m_RaytracingTLAS->GetIndex();
			ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

			ctx->DrawIndexedInstanced(batch.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount, batch.m_Count);
		}
	}

	void ViewRenderer::TraceRadiance(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		renderTargets.m_Normals.m_IsWritable = true;
		renderTargets.m_Normals.m_Format = Render::Format::FORMAT_RGBA16_SNORM;
		renderTargets.m_Normals.Update(view.GetRenderSize(), "ViewRenderTargets::Normals");

		//Transition to UAV the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetRenderTargets().m_SceneBuffer_RenderSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(barrierDesc);
		}

		ctx->TextureBarrier(barriers.size(), barriers.data());

		//ctx->BindPipelineState(renderData.m_TraceRaysPipelineState.get());

		struct alignas(16) ConstantData
		{
			uint32_t SceneTextureDescriptor;
			uint32_t NormalsTextureDescriptor;
		};
		ConstantData data;
		data.SceneTextureDescriptor = renderTargets.m_SceneBuffer_RenderSize.m_TextureViewRW->GetIndex();
		data.NormalsTextureDescriptor = renderTargets.m_Normals.m_TextureViewRW->GetIndex();
		ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

		ctx->DispatchRays(renderData.m_TraceRaysPipelineState.get() , view.GetRenderSize().x, view.GetRenderSize().y);
	}

	void ViewRenderer::RenderSky(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		//Transition to UAV the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneBuffer_RenderSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(barrierDesc);
		}

		ctx->TextureBarrier(barriers.size(), barriers.data());

		ctx->BindPipelineState(m_SkyPSO.get());

		struct alignas(16) ConstantData
		{
			uint32_t SceneTextureDescriptor;
			uint32_t DepthTextureDescriptor;
		};
		ConstantData data;
		data.SceneTextureDescriptor = renderTargets.m_SceneBuffer_RenderSize.m_TextureView->GetIndex();
		data.DepthTextureDescriptor = renderTargets.m_DepthBuffer.m_TextureView->GetIndex();
		ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

		ctx->DispatchThreads({ view.GetRenderSize().x, view.GetRenderSize().y,1 });
	}

	void ViewRenderer::ApplyUpscaling(View& view)
	{
		// TAA, DLSS, FSR, XeSS etc.
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();

		renderTargets.m_SceneBuffer_OutputSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_OutputSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_OutputSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_OutputSize.Update(view.GetRenderSize(), "ViewRenderTargets::SceneBuffer_OutputSize");

		renderTargets.m_SceneHistory.m_IsWritable = true;
		renderTargets.m_SceneHistory.m_IsRenderTarget = true;
		renderTargets.m_SceneHistory.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneHistory.Update(view.GetRenderSize(), "ViewRenderTargets::SceneHistory");

		//TAA Resolve
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		//Transition to UAV the resolve buffer
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneBuffer_OutputSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(barrierDesc);
		}
		//Transition to SRV the scene texture
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneBuffer_RenderSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
			barriers.push_back(barrierDesc);
		}
		//Transition to SRV the velocity texture
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_Velocity.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());

		ctx->BindPipelineState(m_TAAResolvePSO.get());

		struct alignas(16) ConstantData
		{
			uint32_t ResolveTextureDescriptorIndex;
			uint32_t SceneTextureDescriptorIndex;
			uint32_t HistoryTextureDescriptorIndex;
			uint32_t VelocityTextureDescriptorIndex;
			uint32_t pad0;
		};
		ConstantData data;
		data.ResolveTextureDescriptorIndex = renderTargets.m_SceneBuffer_OutputSize.m_TextureViewRW->GetIndex();
		data.SceneTextureDescriptorIndex = renderTargets.m_SceneBuffer_RenderSize.m_TextureView->GetIndex();
		data.HistoryTextureDescriptorIndex = renderTargets.m_SceneHistory.m_TextureView->GetIndex();
		data.VelocityTextureDescriptorIndex = renderTargets.m_Velocity.m_TextureView->GetIndex();
		ctx->BindLocalConstantBuffer(sizeof(data), &data, 0);

		ctx->DispatchThreads(Vector3u(view.GetRenderSize().x, view.GetRenderSize().y, 1));

		//Update history buffer 
		//Transition resolve texture to copy source
		barriers.clear();
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneBuffer_OutputSize.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_SOURCE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_SOURCE;
			barriers.push_back(barrierDesc);
		}
		//Transition history texture to copy dest
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = renderTargets.m_SceneHistory.m_Texture.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_TARGET;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());

		//Perform copy operation
		ctx->CopyTexture(renderTargets.m_SceneHistory.m_Texture.get(), renderTargets.m_SceneBuffer_OutputSize.m_Texture.get());
	}

	void ViewRenderer::ApplyPostEffects(View& view)
	{
		// DoF
		// Bloom
		// Color grading
		// Tonemap + display encoding
	}

	void ViewRenderer::FinalizeFrame(View& view)
	{
		// finalizing work recorded here
		// 
		Render::Context* ctx = Render::Context::GetCurrentContext();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		SET_CONTEXT_MARKER_FUNCTION(ctx);
		// Copy scene texture to view output resource
		// for main view, that would probably be the swapchain backbuffer

		//Resolve buffer should already be in copy source
		//Transition backbuffer to copy dest
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetOutputTarget()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_TARGET;
			ctx->TextureBarrier(barrierDesc);
		}

		//Perform copy operation
		ctx->CopyTexture(view.GetOutputTarget()->GetTexture(), renderTargets.m_SceneBuffer_OutputSize.m_Texture.get());
		
		//Transition output to present
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetOutputTarget()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_PRESENT;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COMMON;
			ctx->TextureBarrier(barrierDesc);
		}
	}

}