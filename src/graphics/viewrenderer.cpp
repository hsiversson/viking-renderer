#include "viewrenderer.h"
#include "view.h"

#include "graphics/material.h"
#include "graphics/mesh.h"
#include "render/context.h"
#include "render/device.h"

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

		//Initializing global shaders
		Render::Device* device = Render::GetDevice();
		m_SkyComputeShader = device->CreateShader("../../../content/shaders/sky.hlsl", L"MainCS", vkr::Render::SHADER_STAGE_COMPUTE);

		Render::PipelineStateDesc skyPSODesc = {};
		skyPSODesc.m_Type = Render::PIPELINE_STATE_TYPE_COMPUTE;
		skyPSODesc.Compute.m_ComputeShader = m_SkyComputeShader.get();

		m_SkyPSO = device->CreatePipelineState(skyPSODesc);

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
			Mat44 WorldToClip;
			uint32_t InstanceDataBufferDescriptorIndex; // Descriptor index to the global buffer where all instance data for the scene is stored
			uint32_t InstanceDataOffsetBufferDescriptorIndex;
			uint32_t MaterialDataBufferDescriptorIndex;
			uint32_t pad0;
			Vector3f CameraWorldPosition;
			uint32_t NumDirectionalLightsInUse;
			DirectionalLight DirectionalLights[2];
		};
		PerSceneConstantData perSceneConstantData = {};
		Mat43 CamWorld = const_cast<Camera&>(view.GetCamera()).GetWorldTransform();
		perSceneConstantData.CameraWorldPosition = Vector3f(CamWorld[9], CamWorld[10], CamWorld[11]);
		perSceneConstantData.WorldToClip = const_cast<Camera&>(view.GetCamera()).GetViewProjection();
		perSceneConstantData.InstanceDataBufferDescriptorIndex = renderData.m_InstanceDataBufferView->GetIndex();
		perSceneConstantData.InstanceDataOffsetBufferDescriptorIndex = renderData.m_InstanceDataOffsetBufferView->GetIndex();
		perSceneConstantData.MaterialDataBufferDescriptorIndex = renderData.m_MaterialDataBuffer.GetBufferView()->GetIndex();

		perSceneConstantData.NumDirectionalLightsInUse = 2;

		perSceneConstantData.DirectionalLights[0].Emission = Vector3f(2.0, 2.0, 8.0);
		perSceneConstantData.DirectionalLights[0].Direction = Vector3f(0.2, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[0].Radius = 1.0f;

		perSceneConstantData.DirectionalLights[1].Emission = Vector3f(8.0, 2.0, 2.0);
		perSceneConstantData.DirectionalLights[1].Direction = Vector3f(-0.2, -0.5, 0.6);
		perSceneConstantData.DirectionalLights[1].Radius = 1.0f;

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
	}

	void ViewRenderer::TraceRadiance(View& view)
	{

	}

	void ViewRenderer::ApplyUpscaling(View& view)
	{
		// TAA, DLSS, FSR, XeSS etc.
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
		// Copy scene texture to view output resource
		// for main view, that would probably be the swapchain backbuffer

		//Transition scene texture to copy source
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetSceneTexture().get();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_COPY_SOURCE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COPY_SOURCE;
			ctx->TextureBarrier(barrierDesc);
		}
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
		ctx->CopyTexture(view.GetOutputTarget()->GetTexture(), view.GetSceneTexture().get());
		
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