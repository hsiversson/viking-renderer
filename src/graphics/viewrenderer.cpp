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

	bool ViewRenderer::Init(Ref<vkr::Render::Device> device)
	{
		m_Device = device;
		// init any renderer subsystems
		// ex. upscalers, water, vegetation, environment, particle/vfx, light culling
		return true;
	}

	void ViewRenderer::RenderView(View& view)
	{
		RenderViewContext renderViewCtx(view);
		//Lets just do a simple forward render for now
		ForwardPass(view);
		//

		UpdateRtScene(view);
		UpdateParticles(view);
		DepthPrepass(view);
		TraceRadiance(view);
		ApplyUpscaling(view);
		ApplyPostEffects(view);
		FinalizeFrame(view);
	}

	void ViewRenderer::ForwardPass(View& view)
	{
		const ViewRenderData& renderData = view.GetRenderData();
		Ref<vkr::Render::Context> ctx = m_Device->GetContext(vkr::Render::CONTEXT_TYPE_GRAPHICS);
		ctx->Begin();

		//Transition to RT the output
		std::vector<Render::TextureBarrierDesc> barriers;
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = static_cast<Render::Texture*>(view.GetOutputTarget()->GetResource());
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
			barriers.push_back(barrierDesc);
		}
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = static_cast<Render::Texture*>(view.GetDepthStencil()->GetResource());
			barrierDesc.m_TargetSync = Render::RESOURCE_STATE_SYNC_DEPTH_STECIL;
			barrierDesc.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_DEPTH_WRITE;
			barrierDesc.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_DEPTH_STENCIL;
			barriers.push_back(barrierDesc);
		}
		ctx->TextureBarrier(barriers.size(), barriers.data());

		std::vector<vkr::Ref<vkr::Render::ResourceDescriptor>> rendertargets;
		rendertargets.push_back(view.GetOutputTarget());
		ctx->BindRenderTargets(rendertargets.data(), rendertargets.size());
		ctx->BindDepthStencil(view.GetDepthStencil());

		const Render::TextureDesc& rtDesc = static_cast<Render::Texture*>(view.GetOutputTarget()->GetResource())->m_TextureDesc;
		ctx->SetViewport(0, 0, rtDesc.Size.x, rtDesc.Size.y);
		ctx->SetScissorRect(0, 0, rtDesc.Size.x, rtDesc.Size.y);

		for (auto& mesh : renderData.m_VisibleMeshes)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(mesh.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers.data(), vertexbuffers.size());
			ctx->BindIndexBuffer(mesh.m_Mesh->GetIndexBuffer());
			ctx->SetPrimitiveTopology(mesh.m_Mesh->GetTopology());
			ctx->BindPSO(mesh.m_Material->GetPipelineState());

			struct alignas(16) ConstantData 
			{
				Mat44 ViewProjection; // 64 bytes
				Mat44 World; // 64 bytes
				Vector4f Color; // 16 bytes
			};
			ConstantData data;
			data.ViewProjection = const_cast<Camera&>(view.GetCamera()).GetViewProjection();
			data.World = mesh.m_Transform;
			data.Color = Vector4f(1, 0, 0, 1);

			auto cbuffer = m_Device->GetTempBuffer(sizeof(ConstantData),sizeof(data), (void*)&data);
			std::vector<vkr::Render::Buffer*> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(cbuffer.m_Buffer);
			offsets.push_back(cbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexed(0, 0);
		}

		//Transition output to present
		{
			Render::TextureBarrierDesc barrierDesc;
			barrierDesc.m_Texture = static_cast<Render::Texture*>(view.GetOutputTarget()->GetResource());
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
		//Render::Device* device = Render::GetDevice();
		//Render::Context* ctx = device->GetContext(Render::CONTEXT_TYPE_GRAPHICS);
		//
		//ctx->SetRenderTarget(nullptr, depthbuffer);
		//
		//for (auto& mesh : renderData.m_VisibleMeshes)
		//{
		//	ctx->BindVertexBuffer(meshVtxBuffer);
		//	ctx->BindIndexBuffer(meshIdxBuffer);
		//	ctx->BindRootConstantBuffers(materialConstantData);
		//	ctx->BindPSO(materialPso);
		//	ctx->Draw();
		//}
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