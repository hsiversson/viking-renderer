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
		std::vector<vkr::Ref<vkr::Render::ResourceDescriptor>> rendertargets;

		rendertargets.push_back(view.GetOutputTarget());
		ctx->BindRenderTargets(rendertargets);

		for (auto& mesh : renderData.m_VisibleMeshes)
		{
			std::vector<vkr::Ref<vkr::Render::Buffer>> vertexbuffers;
			vertexbuffers.push_back(mesh.m_Mesh->GetVertexBuffer());
			ctx->BindVertexBuffers(vertexbuffers);
			ctx->BindIndexBuffer(mesh.m_Mesh->GetIndexBuffer());
			ctx->BindPSO(mesh.m_Material->GetPipelineState());
			//ctx->BindRootConstantBuffers();  //Where do we get the constant buffers from?
		}

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