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

		for (auto& mesh : renderData.m_VisibleMeshes)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(mesh.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers.data(), vertexbuffers.size());
			ctx->BindIndexBuffer(mesh.m_Mesh->GetIndexBuffer());
			ctx->SetPrimitiveTopology(mesh.m_Mesh->GetTopology());
			ctx->BindPSO(mesh.m_Material->GetDefaultPipelineState());

			struct alignas(16) ConstantData
			{
				Mat44 ViewProjection; // 64 bytes
				Mat44 World; // 64 bytes
				Vector3f Color;
				uint32_t TextureDescriptor; // 16 bytes
			};
			ConstantData data;
			data.ViewProjection = const_cast<Camera&>(view.GetCamera()).GetViewProjection();
			data.World = mesh.m_Transform;
			data.Color = Vector3f(1, 0, 0);
			data.TextureDescriptor = mesh.m_Material->GetTexture() ? mesh.m_Material->GetTexture()->GetIndex() : 0;

			auto cbuffer = Render::GetDevice()->GetTempBuffer(sizeof(ConstantData),sizeof(data), (void*)&data);
			std::vector<vkr::Render::Buffer*> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(cbuffer.m_Buffer);
			offsets.push_back(cbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexed(mesh.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount);
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
		
		for (auto& mesh : renderData.m_VisibleMeshes)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(mesh.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers.data(), vertexbuffers.size());
			ctx->BindIndexBuffer(mesh.m_Mesh->GetIndexBuffer());
			ctx->SetPrimitiveTopology(mesh.m_Mesh->GetTopology());
			ctx->BindPSO(mesh.m_Material->GetDepthPipelineState());

			struct alignas(16) ConstantData
			{
				Mat44 ViewProjection; // 64 bytes
				Mat44 World; // 64 bytes
				Vector3f Color; 
				uint32_t TextureDescriptor; // 16 bytes
			};
			ConstantData data;
			data.ViewProjection = const_cast<Camera&>(view.GetCamera()).GetViewProjection();
			data.World = mesh.m_Transform;
			data.Color = Vector3f(1, 0, 0);
			data.TextureDescriptor = mesh.m_Material->GetTexture() ? mesh.m_Material->GetTexture()->GetIndex() : 0; 

			auto cbuffer = Render::GetDevice()->GetTempBuffer(sizeof(ConstantData), sizeof(data), (void*)&data);
			std::vector<vkr::Render::Buffer*> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(cbuffer.m_Buffer);
			offsets.push_back(cbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexed(mesh.m_Mesh->GetIndexBuffer()->GetDesc().m_ElementCount);
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