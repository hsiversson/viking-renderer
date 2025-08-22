#include "viewrenderer.h"
#include "view.h"

#include "application/appsettings.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "render/context.h"
#include "render/device.h"
#include "render/nvdlss.h"
#include "render/nvstreamline.h"

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
		m_StaticVelShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/staticvel.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc staticVelPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		staticVelPSODesc.Compute.m_ComputeShader = m_StaticVelShader.get();
		m_StaticVelPSO = device->CreatePipelineState(staticVelPSODesc);

		//Sky
		
		m_SkyComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/sky.hlsl"), L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc skyPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		skyPSODesc.Compute.m_ComputeShader = m_SkyComputeShader.get();
		m_SkyPSO = device->CreatePipelineState(skyPSODesc);

		//TAA
				
		m_TAAResolveComputeShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/taa.hlsl"), L"ResolveCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc taaPSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		taaPSODesc.Compute.m_ComputeShader = m_TAAResolveComputeShader.get();
		m_TAAResolvePSO = device->CreatePipelineState(taaPSODesc);

		// Raytrace
		m_RaytraceShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/tracerays.hlsl"), L"Main", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc raytracePSODesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_COMPUTE);
		raytracePSODesc.Compute.m_ComputeShader = m_RaytraceShader.get();
		m_RaytracePSO = device->CreatePipelineState(raytracePSODesc);

		return true;
	}

	void ViewRenderer::RenderView(View& view)
	{
		View* viewPtr = &view; 
		viewPtr->BeginRender();

		// no need to split into multiple tasks yet...
		Render::QueueGraphicsTask([this, viewPtr]() mutable 
			{ 
				PreRenderUpdates(*viewPtr); 
				DepthPrepass(*viewPtr);
				StaticVelocity(*viewPtr);
				TraceRadiance(*viewPtr);
				ApplyUpscaling(*viewPtr);
				FinalizeFrame(*viewPtr);
			});

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
		auto instanceDataBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, std::max(renderData.m_InstanceData.size(), 256ull), renderData.m_InstanceData.size(), renderData.m_InstanceData.data());

		Render::BufferViewDesc instanceDataBufferDesc = {}; // Byteaddressbuffer
		instanceDataBufferDesc.m_ElementStart = instanceDataBuffer.m_Offset;
		instanceDataBufferDesc.m_ElementCount = std::max(renderData.m_InstanceData.size(), 256ull);
		instanceDataBufferDesc.m_ElementSize = 1;
		instanceDataBufferDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAW;
		instanceDataBufferDesc.m_Format = Render::FORMAT_UNKNOWN;
		renderData.m_InstanceDataBufferView = Render::GetDevice()->CreateBufferView(instanceDataBufferDesc, instanceDataBuffer.m_Buffer);

		//Fill in the instance data indices
		auto instanceDataOffsetBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, std::max(renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), 4ull), renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), renderData.m_InstanceDataOffsetBuffer.data());

		Render::BufferViewDesc instanceDataOffsetBufferDesc = {}; // Typed uint buffer
		instanceDataOffsetBufferDesc.m_ElementStart = instanceDataOffsetBuffer.m_Offset / sizeof(uint32_t);
		instanceDataOffsetBufferDesc.m_ElementCount = std::max(renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), 4ull);
		instanceDataOffsetBufferDesc.m_Usage = Render::BUFFER_VIEW_USAGE_TYPED;
		instanceDataOffsetBufferDesc.m_Format = Render::FORMAT_R32_UINT;
		renderData.m_InstanceDataOffsetBufferView = Render::GetDevice()->CreateBufferView(instanceDataOffsetBufferDesc, instanceDataOffsetBuffer.m_Buffer);

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

		perSceneConstantData.CurrentJitter = renderData.m_CameraData.CurrentJitter;
		perSceneConstantData.PrevJitter = renderData.m_CameraData.PrevJitter;
		perSceneConstantData.FrameIndex = renderData.m_FrameIndex;
		perSceneConstantData.DeltaTime = renderData.m_DeltaTime;
		perSceneConstantData.ElapsedTime = renderData.m_ElapsedTime;

		perSceneConstantData.CameraWorldPosition = Vector3f(renderData.m_CameraData.CameraWorldMatrix[9], renderData.m_CameraData.CameraWorldMatrix[10], renderData.m_CameraData.CameraWorldMatrix[11]);
		perSceneConstantData.InvView = renderData.m_CameraData.InvViewMatrix;
		perSceneConstantData.Projection = renderData.m_CameraData.ProjectionMatrix;
		perSceneConstantData.InvProjection = renderData.m_CameraData.InvProjectionMatrix;
		perSceneConstantData.ViewProjection = renderData.m_CameraData.ViewProjectionMatrix;
		perSceneConstantData.InvViewProjection = renderData.m_CameraData.InvViewProjectionMatrix;
		perSceneConstantData.PrevViewProjection = renderData.m_CameraData.PrevViewProjectionMatrix;

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
		renderTargets.m_DepthBuffer.Update(renderData.m_RenderSize, "ViewRenderTargets::DepthBuffer");

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
		renderTargets.m_SceneBuffer_RenderSize.Update(renderData.m_RenderSize, "ViewRenderTargets::SceneBuffer_RenderSize");

		renderTargets.m_Velocity.m_IsWritable = true;
		renderTargets.m_Velocity.m_IsRenderTarget = true;
		renderTargets.m_Velocity.m_Format = Render::Format::FORMAT_RG16_FLOAT;
		uint32_t nanBits = 0xffffffff;
		float nanValue = *reinterpret_cast<float*>(&nanBits);
		renderTargets.m_Velocity.m_ClearValue = { nanValue, nanValue, nanValue, nanValue };
		renderTargets.m_Velocity.Update(renderData.m_RenderSize, "ViewRenderTargets::Velocity");

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

			ctx->DispatchThreads({ renderData.m_RenderSize.x, renderData.m_RenderSize.y, 1 });
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
		renderTargets.m_SceneBuffer_RenderSize.Update(renderData.m_RenderSize, "ViewRenderTargets::SceneBuffer_RenderSize");

		renderTargets.m_SceneBuffer_OutputSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_OutputSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_OutputSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_OutputSize.Update(renderData.m_OutputSize, "ViewRenderTargets::SceneBuffer_OutputSize");

		renderTargets.m_Velocity.m_IsWritable = true;
		renderTargets.m_Velocity.m_IsRenderTarget = true;
		renderTargets.m_Velocity.m_Format = Render::Format::FORMAT_RG16_FLOAT;
		renderTargets.m_Velocity.m_ClearValue = { FLT_MAX,FLT_MAX,FLT_MAX ,FLT_MAX };
		renderTargets.m_Velocity.Update(renderData.m_RenderSize, "ViewRenderTargets::Velocity");

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

		if (!renderData.m_TraceRaysPipelineState)
			return;

		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		renderTargets.m_Normals.m_IsWritable = true;
		renderTargets.m_Normals.m_Format = Render::Format::FORMAT_RGBA16_SNORM;
		renderTargets.m_Normals.Update(renderData.m_RenderSize, "ViewRenderTargets::Normals");

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

		ctx->DispatchRays(renderData.m_TraceRaysPipelineState.get() , renderData.m_RenderSize.x, renderData.m_RenderSize.y);
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

		ctx->DispatchThreads({ renderData.m_RenderSize.x, renderData.m_RenderSize.y,1 });
	}

	void ViewRenderer::ApplyUpscaling(View& view)
	{
		// TAA, DLSS, FSR, XeSS etc.
		const ViewRenderData& renderData = view.GetRenderData();
		ViewRenderTargets& renderTargets = view.GetRenderTargets();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		SET_CONTEXT_MARKER_FUNCTION(ctx);

		renderTargets.m_SceneBuffer_OutputSize.m_IsWritable = true;
		renderTargets.m_SceneBuffer_OutputSize.m_IsRenderTarget = true;
		renderTargets.m_SceneBuffer_OutputSize.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
		renderTargets.m_SceneBuffer_OutputSize.Update(renderData.m_OutputSize, "ViewRenderTargets::SceneBuffer_OutputSize");

		//Transitions
		{
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
		}

		if (AppSettings::GetAppSettings()->GetGraphicsSettings().m_AAMethod == TAA)
		{
			renderTargets.m_SceneHistory.m_IsWritable = true;
			renderTargets.m_SceneHistory.m_IsRenderTarget = true;
			renderTargets.m_SceneHistory.m_Format = Render::Format::FORMAT_RGBA16_FLOAT;
			renderTargets.m_SceneHistory.Update(renderData.m_RenderSize, "ViewRenderTargets::SceneHistory");

			//TAA Resolve

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

			ctx->DispatchThreads(Vector3u(renderData.m_RenderSize.x, renderData.m_RenderSize.y, 1));

			//Update history buffer 

			//Transitions
			{
				//Transition resolve texture to copy source
				std::vector<Render::TextureBarrierDesc> barriers;
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
			}

			//Perform copy operation
			ctx->CopyTexture(renderTargets.m_SceneHistory.m_Texture.get(), renderTargets.m_SceneBuffer_OutputSize.m_Texture.get());
		}
		else //Using DLSS
		{
			view.GetDLSS().Upscale(view, ctx);
			{
				std::vector<Render::TextureBarrierDesc> barriers;
				{
					Render::TextureBarrierDesc barrierDesc;
					barrierDesc.m_Texture = renderTargets.m_SceneBuffer_OutputSize.m_Texture.get();
					barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
					barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_SOURCE;
					barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_SOURCE;
					barriers.push_back(barrierDesc);
				}
				ctx->TextureBarrier(barriers.size(), barriers.data());
			}
		}

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