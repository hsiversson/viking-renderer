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

	bool ViewRenderer::Init()
	{
		// init any renderer subsystems
		// ex. upscalers, water, vegetation, environment, particle/vfx, light culling
		return true;
	}

	void ViewRenderer::RenderView(View& view)
	{
		RenderViewContext renderViewCtx(view);

		const ViewRenderData& renderData = view.GetRenderData();

		//Fill in the instance data (later we can just keep this as a normal buffer instead of temp that we need to rebuild per frame)
		auto instancedatabuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, renderData.m_InstanceData.size(), renderData.m_InstanceData.size(), renderData.m_InstanceData.data());
		//Oh god what ive done, const fucking
		uint32_t instancedatastart = (uint32_t)(instancedatabuffer.m_Offset / 4.0);
		uint32_t instancedataend = instancedatastart + (uint32_t)renderData.m_InstanceData.size() / 4;
		const_cast<ViewRenderData&>(renderData).m_InstanceDataBufferView = Render::GetDevice()->CreateBufferView({ instancedatastart, instancedataend, 1, false, Render::Raw }, instancedatabuffer.m_Buffer);


		//Fill in the instance data indices
		auto instancedataoffsetbuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_STAGING, renderData.m_InstanceDataOffsetBuffer.size()*sizeof(uint32_t), renderData.m_InstanceDataOffsetBuffer.size() * sizeof(uint32_t), renderData.m_InstanceDataOffsetBuffer.data());
		uint32_t instancedataoffsetstart = (uint32_t)(instancedataoffsetbuffer.m_Offset / sizeof(uint32_t));
		uint32_t instancedataoffsetend = instancedataoffsetstart + (uint32_t)renderData.m_InstanceDataOffsetBuffer.size();
		//Oh god what ive done, const fucking
		const_cast<ViewRenderData&>(renderData).m_InstanceDataOffsetBufferView = Render::GetDevice()->CreateBufferView({ instancedataoffsetstart, instancedataoffsetend, 1, false, Render::Typed }, instancedatabuffer.m_Buffer);

		//Construct the per scene constant buffer
		struct alignas(16) PerSceneConstantData
		{
			Mat44 ViewProjection;
			uint32_t InstanceDataBufferDescriptorIndex; // Descriptor index to the global buffer where all instance data for the scene is stored
			uint32_t InstanceDataOffsetBufferDescriptorIndex;
		};
		PerSceneConstantData persceneconstantdata;
		persceneconstantdata.ViewProjection = const_cast<Camera&>(view.GetCamera()).GetViewProjection();
		persceneconstantdata.InstanceDataBufferDescriptorIndex = renderData.m_InstanceDataBufferView->GetIndex();
		persceneconstantdata.InstanceDataOffsetBufferDescriptorIndex = renderData.m_InstanceDataOffsetBufferView->GetIndex();
		const_cast<ViewRenderData&>(renderData).m_PerSceneConstantBuffer = Render::GetDevice()->GetTempBuffer(Render::TEMP_BUFFER_USAGE_CONSTANTS, sizeof(PerSceneConstantData), sizeof(PerSceneConstantData), &persceneconstantdata);
		
		UpdateRtScene(view);
		UpdateParticles(view);
		DepthPrepass(view);
		//Lets just do a simple forward render for now, remove when raytracing is in place
		ForwardPass(view);
		//
		TraceRadiance(view);
		ApplyUpscaling(view);
		ApplyPostEffects(view);
		FinalizeFrame(view);
	}

	void ViewRenderer::ForwardPass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Ref<vkr::Render::Context> ctx = Render::GetDevice()->GetContext(vkr::Render::CONTEXT_TYPE_GRAPHICS);
		ctx->Begin();

		//Transition to RT the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetOutputTarget()->GetTexture();
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
		rendertargets.push_back(view.GetOutputTarget());

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
			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;

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

		//Transition output to present
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = view.GetOutputTarget()->GetTexture();
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_ALL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_PRESENT;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_COMMON;
			ctx->TextureBarrier(barrierDesc);
		}
		ctx->End();
		ctx->Flush();
	}

	void ViewRenderer::UpdateRtScene(View& view)
	{

	}

	void ViewRenderer::UpdateParticles(View& view)
	{

	}

	void ViewRenderer::DepthPrepass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Ref<vkr::Render::Context> ctx = Render::GetDevice()->GetContext(vkr::Render::CONTEXT_TYPE_GRAPHICS);
		ctx->Begin();

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
			};
			ConstantData data;
			data.m_BatchStart = batch.m_StartOffset;

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
		ctx->End();
		ctx->Flush();
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
		// copy to view output resource?
		// for main view, that would probably be the swapchain backbuffer
	}
}