#include "viewrenderer.h"
#include "view.h"

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
		UpdateRtScene(view);
		UpdateParticles(view);
		DepthPrepass(view);
		TraceRadiance(view);
		ApplyUpscaling(view);
		ApplyPostEffects(view);
		FinalizeFrame(view);
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