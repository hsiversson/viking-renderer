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
		std::vector<vkr::Ref<vkr::Render::ResourceDescriptor>> rendertargets;

		rendertargets.push_back(view.GetOutputTarget());
		ctx->BindRenderTargets(rendertargets.data(), rendertargets.size());
		ctx->BindDepthStencil(view.GetDepthStencil());

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
			data.Color = Vector4f(1, 0, 0, 0);

			auto cbuffer = m_Device->GetTempBuffer(sizeof(ConstantData),sizeof(data), (void*)&data);
			std::vector<vkr::Render::Buffer*> buffers;
			std::vector<uint64_t> offsets;
			buffers.push_back(cbuffer.m_Buffer);
			offsets.push_back(cbuffer.m_Offset);
			ctx->BindRootConstantBuffers(buffers.data(), buffers.size(), offsets.data());
			ctx->DrawIndexed(0, 0);
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