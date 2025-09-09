#pragma once

#include "core/types.h"

namespace vkr::Render
{
	class Device;
	class Shader;
	class PipelineState;
	class RenderTargetView;
	class RenderTaskEvent;
	class Texture;
	class TextureView;
}

namespace vkr::Graphics
{
	class View;
	class SkyRenderer;

	class ViewRenderer
	{
	public:
		ViewRenderer();
		~ViewRenderer();

		bool Init();

		void RenderView(View* view);

	private:
		
		void PreRenderUpdates(View* view);
		void UpdateParticles(View* view);

		void DepthPrepass(View* view);
		void StaticVelocity(View* view);
		void ForwardPass(View* view);
		void TraceRadiance(View* view);

		void RenderSky(View* view);

		void ApplyUpscaling(View* view);
		void ApplyPostEffects(View* view);
		void FinalizeFrame(View* view);

	private:
		// example sub systems
		// UniquePtr<Environment> m_Environment;
		// UniquePtr<VegetationSystem> m_VegetationSystem;
		// UniquePtr<VfxSimulator> m_VfxSimulator;

		// Global shader cache??
		Ref<Render::Shader> m_StaticVelShader;
		Ref<Render::PipelineState> m_StaticVelPSO;
		Ref<Render::Shader> m_SkyComputeShader;
		Ref<Render::PipelineState> m_SkyPSO;
		Ref<Render::Shader> m_RaytraceShader;
		Ref<Render::PipelineState> m_RaytracePSO;

		// TAA
		Ref<Render::Shader> m_TAAResolveComputeShader;
		Ref<Render::PipelineState> m_TAAResolvePSO;

		//RT

		//Sky
		
		// Post-processing
		Ref<Render::PipelineState> m_ClearHistogramPSO;
		Ref<Render::PipelineState> m_BuildHistogramPSO;
		Ref<Render::PipelineState> m_ComputeExposurePSO;
		Ref<Render::PipelineState> m_TonemapPSO;

		// Sub-systems
		UniquePtr<SkyRenderer> m_SkyRenderer;

	};
}