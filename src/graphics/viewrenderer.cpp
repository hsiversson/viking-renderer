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

	bool ViewRenderer::Init(View& view)
	{
		// init any renderer subsystems
		// ex. upscalers, water, vegetation, environment, particle/vfx, light culling

		//Initializing global shaders/resources

		//Sky
		Render::Device* device = Render::GetDevice();
		m_SkyComputeShader = device->CreateShader("../../../content/shaders/sky.hlsl", L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);

		Render::PipelineStateDesc skyPSODesc = {};
		skyPSODesc.m_Type = Render::PIPELINE_STATE_TYPE_COMPUTE;
		skyPSODesc.Compute.m_ComputeShader = m_SkyComputeShader.get();

		m_SkyPSO = device->CreatePipelineState(skyPSODesc);

		//TAA
		m_TAAHistoryBuffer = device->CreateTexture(view.GetSceneTexture()->m_TextureDesc);
		m_TAAResolveBuffer = device->CreateTexture(view.GetSceneTexture()->m_TextureDesc);

		Render::TextureViewDesc SRVViewDesc;
		SRVViewDesc.m_Mip = 0;
		SRVViewDesc.m_Writable = false;
		Render::TextureViewDesc UAVViewDesc;
		SRVViewDesc.m_Mip = 0;
		SRVViewDesc.m_Writable = true;
		m_TAAHistorySRVView = device->CreateTextureView(SRVViewDesc, m_TAAHistoryBuffer);
		m_TAAHistoryUAVView = device->CreateTextureView(UAVViewDesc, m_TAAHistoryBuffer);
		m_TAAResolveSRVView = device->CreateTextureView(SRVViewDesc, m_TAAResolveBuffer);
		m_TAAResolveUAVView = device->CreateTextureView(UAVViewDesc, m_TAAResolveBuffer);

		m_TAAResolveComputeShader = device->CreateShader("../../../content/shaders/taa.hlsl", L"ResolveCS", vkr::Render::SHADER_STAGE_COMPUTE);
		Render::PipelineStateDesc taaPSODesc = {};
		taaPSODesc.m_Type = Render::PIPELINE_STATE_TYPE_COMPUTE;
		taaPSODesc.Compute.m_ComputeShader = m_TAAResolveComputeShader.get();

		m_TAAResolvePSO = device->CreatePipelineState(taaPSODesc);
		return true;
	}

	void ViewRenderer::RenderView(View& view)
	{
		View* viewPtr = &view; 
		viewPtr->BeginRender();
		Render::QueueGraphicsTask([this, viewPtr]() mutable { UpdateSceneData(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { UpdateRtScene(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { DepthPrepass(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { ForwardPass(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { RenderSky(*viewPtr); });
		Render::QueueGraphicsTask([this, viewPtr]() mutable { ApplyUpscaling(*viewPtr); });
		Ref<Render::RenderTaskEvent> lastRenderEvent = Render::QueueGraphicsTask([this, viewPtr]() mutable { FinalizeFrame(*viewPtr); });
		viewPtr->EndRender();

		//UpdateRtScene(view);
		//UpdateParticles(view);

		//DepthPrepass(view);

		//Lets just do a simple forward render for now, remove when raytracing is in place
		//ForwardPass(view);

		//
		//TraceRadiance(view);
		//RenderSky(view);
		//ApplyUpscaling(view);
		//ApplyPostEffects(view);
		//FinalizeFrame(view);
	}

	void ViewRenderer::RenderSky(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		
		// Create an UAV from the backbuffer
		Render::TextureViewDesc sceneViewDesc;
		sceneViewDesc.m_Mip = 0;
		sceneViewDesc.m_Writable = true;
		m_SceneTextureUAVView = Render::GetDevice()->CreateTextureView(sceneViewDesc, view.GetSceneTexture()); //Is efficient 

		//Transition to UAV the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetSceneTexture().get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(barrierDesc);
		}

		m_DepthSRVView = Render::GetDevice()->CreateTextureView(Render::TextureViewDesc(), view.GetDepthBufferTexture());
		// Depth should be already transitioned to read state by now
		
		ctx->BindPSO(m_SkyPSO);

		struct alignas(16) ConstantData
		{
			uint32_t SceneTextureDescriptor;
			uint32_t DepthTextureDescriptor;
		};
		ConstantData data;
		data.SceneTextureDescriptor = m_SceneTextureUAVView->GetIndex();
		data.DepthTextureDescriptor = m_DepthSRVView->GetIndex();

		//Per batch buffer
		auto perbatchbuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(ConstantData), sizeof(data), (void*)&data);
		std::vector<Ref<vkr::Render::Buffer>> buffers;
		std::vector<uint64_t> offsets;
		buffers.push_back(renderData.m_PerSceneConstantBuffer.m_Buffer);
		offsets.push_back(renderData.m_PerSceneConstantBuffer.m_Offset);
		buffers.push_back(perbatchbuffer.m_Buffer);
		offsets.push_back(perbatchbuffer.m_Offset);
		ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());

		uint32_t GroupsX = ceil(view.GetRenderSize().x / 8);
		uint32_t GroupsY = ceil(view.GetRenderSize().y / 8);
		ctx->Dispatch({GroupsX, GroupsY, 1});
	}

	void ViewRenderer::ForwardPass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Render::Context* ctx = Render::Context::GetCurrentContext();
		ctx->SetMarker("ViewRenderer::ForwardPass");

		Render::RenderTargetViewDesc sceneRTViewDesc;
		sceneRTViewDesc.m_Mip = 0;
		Ref<Render::RenderTargetView> sceneRTView = Render::GetDevice()->CreateRenderTargetView(sceneRTViewDesc, view.GetSceneTexture());

		//Transition to RT the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetSceneTexture().get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
			barriers.push_back(barrierDesc);
		}
		//We will only read from DS now that depths are written to
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetDepthBuffer()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_READ;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL_READ;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());


		std::vector<vkr::Ref<vkr::Render::RenderTargetView>> rendertargets;
		rendertargets.push_back(sceneRTView);

		ctx->ClearRenderTargets(rendertargets.data(), rendertargets.size());
		//ctx->ClearDepthStencil(view.GetDepthBuffer(), 0.0f);

		ctx->BindRenderTargets(rendertargets.data(), rendertargets.size());
		ctx->BindDepthStencil(view.GetDepthBuffer());

		const Render::TextureDesc& rtDesc = view.GetOutputTarget()->GetTexture()->m_TextureDesc;
		ctx->SetViewport(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		ctx->SetScissorRect(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);

		for (auto& batch : renderData.m_ForwardPassData.m_InstanceBatches)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(batch.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers.data(), vertexbuffers.size());
			ctx->BindIndexBuffer(batch.m_Mesh->GetIndexBuffer());
			ctx->SetPrimitiveTopology(batch.m_Mesh->GetTopology());
			ctx->BindPSO(batch.m_PSO);

			struct alignas(16) ConstantData
			{
				uint32_t m_BatchStart;
				uint32_t RaytracingSceneDescriptor;

			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;
			data.RaytracingSceneDescriptor = renderData.m_RaytracingTLAS->GetIndex();

			//Per batch buffer
			auto perbatchbuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(ConstantData), sizeof(data), (void*)&data);
			std::vector<Ref<vkr::Render::Buffer>> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(renderData.m_PerSceneConstantBuffer.m_Buffer);
			offsets.push_back(renderData.m_PerSceneConstantBuffer.m_Offset);
			buffers.push_back(perbatchbuffer.m_Buffer);
			offsets.push_back(perbatchbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexedInstanced(batch.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount, batch.m_Count);
		}

		ctx->EndMarker();
	}

	void ViewRenderer::UpdateSceneData(View& view)
	{
		ViewRenderData& renderData = view.GetMutableRenderData();

		renderData.m_MaterialDataBuffer.PrepareBuffer();

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
			Mat44 ViewProjectionNoJitter;
			uint32_t InstanceDataBufferDescriptorIndex; // Descriptor index to the global buffer where all instance data for the scene is stored
			uint32_t InstanceDataOffsetBufferDescriptorIndex;
			uint32_t MaterialDataBufferDescriptorIndex;
			uint32_t pad0;
			Vector3f CameraWorldPosition;
			uint32_t NumDirectionalLightsInUse;
			DirectionalLight DirectionalLights[2];
		};

		Mat44 ProjectionNoJitter = const_cast<Camera&>(view.GetCamera()).GetProjection();
		//Select a new jitter offset for TAA for this frame
		int jitterIdx = (m_CurrentJitterIndex++) % 16;
		Vector2f jitter = (JitterHaltonSequence[jitterIdx] - 0.5f) / Vector2f(view.GetRenderSize()) * 2.0f;
		Mat44 Projection = ProjectionNoJitter;
		Projection[8] = jitter.x;
		Projection[9] = jitter.y;

		PerSceneConstantData perSceneConstantData = {};
		Mat43 CamWorld = const_cast<Camera&>(view.GetCamera()).GetWorldTransform();
		perSceneConstantData.CameraWorldPosition = Vector3f(CamWorld[9], CamWorld[10], CamWorld[11]);
		perSceneConstantData.View = const_cast<Camera&>(view.GetCamera()).GetView();
		perSceneConstantData.InvView = Inverse(perSceneConstantData.View);
		perSceneConstantData.Projection = Projection;
		perSceneConstantData.InvProjection = Inverse(perSceneConstantData.Projection);
		perSceneConstantData.ViewProjection = perSceneConstantData.View * perSceneConstantData.Projection;
		perSceneConstantData.InvViewProjection = Inverse(perSceneConstantData.ViewProjection);
		perSceneConstantData.ViewProjectionNoJitter = perSceneConstantData.View * ProjectionNoJitter;
		perSceneConstantData.InstanceDataBufferDescriptorIndex = renderData.m_InstanceDataBufferView->GetIndex();
		perSceneConstantData.InstanceDataOffsetBufferDescriptorIndex = renderData.m_InstanceDataOffsetBufferView->GetIndex();
		perSceneConstantData.MaterialDataBufferDescriptorIndex = renderData.m_MaterialDataBuffer.GetBufferView()->GetIndex();

		perSceneConstantData.NumDirectionalLightsInUse = 2;

		perSceneConstantData.DirectionalLights[0].Emission = Vector3f(2.0, 2.0, 8.0);
		perSceneConstantData.DirectionalLights[0].Direction = Vector3f(0.4, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[0].Radius = 0.02f;

		perSceneConstantData.DirectionalLights[1].Emission = Vector3f(8.0, 2.0, 2.0);
		perSceneConstantData.DirectionalLights[1].Direction = Vector3f(-0.4, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[1].Radius = 0.02f;

		renderData.m_PerSceneConstantBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(PerSceneConstantData), sizeof(PerSceneConstantData), &perSceneConstantData);

	}

	void ViewRenderer::UpdateRtScene(View& view)
	{
		ViewRenderData& renderData = view.GetMutableRenderData();

		Ref<Render::Buffer> rtTLAS = Render::GetDevice()->CreateTLAS(renderData.m_RaytracingInstances.size(), renderData.m_RaytracingInstances.data());

		Render::BufferViewDesc rtTLASDesc = {};
		rtTLASDesc.m_Usage = Render::BUFFER_VIEW_USAGE_RAYTRACING_ACCELERATION_STRUCTURE;
		renderData.m_RaytracingTLAS = Render::GetDevice()->CreateBufferView(rtTLASDesc, rtTLAS);
	}

	void ViewRenderer::UpdateParticles(View& view)
	{

	}

	void ViewRenderer::DepthPrepass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Render::Context* ctx = Render::Context::GetCurrentContext();

		ctx->InsertWait(renderData.m_RaytracingTLAS->GetBuffer()->GetGpuPending());

		ctx->SetMarker("ViewRenderer::DepthPrepass");

		//Transition DS to write
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetDepthBuffer()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_DEPTH_STENCIL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL_WRITE;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());

		ctx->ClearDepthStencil(view.GetDepthBuffer(), 0.0f);

		std::vector<vkr::Ref<vkr::Render::RenderTargetView>> rendertargets;
		ctx->BindRenderTargets(rendertargets.data(), rendertargets.size());
		ctx->BindDepthStencil(view.GetDepthBuffer());

		const Render::TextureDesc& rtDesc = view.GetDepthBuffer()->GetTexture()->m_TextureDesc;
		ctx->SetViewport(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		ctx->SetScissorRect(0, 0, rtDesc.m_Size.x, rtDesc.m_Size.y);
		
		for (auto& batch : renderData.m_DepthPassData.m_InstanceBatches)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(batch.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers.data(), vertexbuffers.size());
			ctx->BindIndexBuffer(batch.m_Mesh->GetIndexBuffer());
			ctx->SetPrimitiveTopology(batch.m_Mesh->GetTopology());
			ctx->BindPSO(batch.m_PSO);

			struct alignas(16) ConstantData
			{
				uint32_t m_BatchStart;
				uint32_t RaytracingSceneDescriptor;
			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;
			data.RaytracingSceneDescriptor = renderData.m_RaytracingTLAS->GetIndex();

			//Per batch buffer
			auto perbatchbuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(ConstantData), sizeof(data), (void*)&data);
			std::vector<Ref<vkr::Render::Buffer>> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(renderData.m_PerSceneConstantBuffer.m_Buffer);
			offsets.push_back(renderData.m_PerSceneConstantBuffer.m_Offset);
			buffers.push_back(perbatchbuffer.m_Buffer);
			offsets.push_back(perbatchbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexedInstanced(batch.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount, batch.m_Count);
		}

		ctx->EndMarker();
	}

	void ViewRenderer::TraceRadiance(View& view)
	{

	}

	void ViewRenderer::ApplyUpscaling(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		// TAA, DLSS, FSR, XeSS etc.

		//TAA Resolve
		Render::Context* ctx = Render::Context::GetCurrentContext();
		ctx->SetMarker("ViewRenderer::ApplyUpscaling");
		//Transition to UAV the resolve buffer
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = m_TAAResolveBuffer.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			barriers.push_back(barrierDesc);
		}
		//Transition to SRV the scene texture
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetSceneTexture().get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_COMPUTE;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_READ;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_READ_RESOURCE;
			barriers.push_back(barrierDesc);
		}

		ctx->BindPSO(m_TAAResolvePSO);

		// Create an SRV from the backbuffer
		Render::TextureViewDesc sceneViewDesc;
		sceneViewDesc.m_Mip = 0;
		sceneViewDesc.m_Writable = false;
		m_SceneTextureSRVView = Render::GetDevice()->CreateTextureView(sceneViewDesc, view.GetSceneTexture()); //Is efficient 

		struct alignas(16) ConstantData
		{
			uint32_t ResolveTextureDescriptorIndex;
			uint32_t SceneTextureDescriptorIndex;
			uint32_t HistoryTextureDescriptorIndex;
			uint32_t pad0;
		};
		ConstantData data;
		data.ResolveTextureDescriptorIndex = m_TAAResolveUAVView->GetIndex();
		data.SceneTextureDescriptorIndex = m_SceneTextureSRVView->GetIndex();
		data.HistoryTextureDescriptorIndex = m_TAAHistorySRVView->GetIndex();

		//Per batch buffer
		auto perbatchbuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(ConstantData), sizeof(data), (void*)&data);
		std::vector<Ref<vkr::Render::Buffer>> buffers;
		std::vector<uint64_t> offsets;
		buffers.push_back(renderData.m_PerSceneConstantBuffer.m_Buffer);
		offsets.push_back(renderData.m_PerSceneConstantBuffer.m_Offset);
		buffers.push_back(perbatchbuffer.m_Buffer);
		offsets.push_back(perbatchbuffer.m_Offset);
		ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());

		ctx->DispatchThreads(Vector3u(view.GetRenderSize().x, view.GetRenderSize().y, 1));

		//Update history buffer 
		//Transition resolve texture to copy source
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = m_TAAResolveBuffer.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_SOURCE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_SOURCE;
			ctx->TextureBarrier(barrierDesc);
		}
		//Transition history texture to copy dest
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = m_TAAHistoryBuffer.get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_TARGET;
			ctx->TextureBarrier(barrierDesc);
		}

		//Perform copy operation
		ctx->CopyTexture(m_TAAHistoryBuffer.get(), m_TAAResolveBuffer.get());
		ctx->EndMarker();
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
		ctx->SetMarker("ViewRenderer::FinalizeFrame");
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
		ctx->CopyTexture(view.GetOutputTarget()->GetTexture(), m_TAAResolveBuffer.get());
		
		//Transition output to present
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetOutputTarget()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_PRESENT;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COMMON;
			ctx->TextureBarrier(barrierDesc);
		}

		ctx->EndMarker();
	}
}