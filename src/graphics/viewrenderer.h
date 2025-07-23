#pragma once

#include "core/types.h"

namespace vkr::Render
{
	class Device;
	class Shader;
	class PipelineState;
	class RenderTargetView;
	class Texture;
	class TextureView;
}

namespace vkr::Graphics
{
	class View;
	class ViewRenderer
	{
	public:
		ViewRenderer();
		~ViewRenderer();

		bool Init(View& view);

		void RenderView(View& view);

	private:
		void ForwardPass(View& view);
		void UpdateSceneData(View& view);
		void UpdateRtScene(View& view);
		void UpdateParticles(View& view);

		void DepthPrepass(View& view);

		void TraceRadiance(View& view);

		void RenderSky(View& view);

		void ApplyUpscaling(View& view);
		void ApplyPostEffects(View& view);
		void FinalizeFrame(View& view);

	private:
		// example sub systems
		// UniquePtr<Environment> m_Environment;
		// UniquePtr<VegetationSystem> m_VegetationSystem;
		// UniquePtr<VfxSimulator> m_VfxSimulator;

		//Global shader cache??
		Ref<Render::Shader> m_SkyComputeShader;
		Ref<Render::PipelineState> m_SkyPSO;

		Ref<Render::TextureView> m_SceneTextureUAVView;
		Ref<Render::TextureView> m_SceneTextureSRVView;
		Ref<Render::TextureView> m_DepthSRVView;

		//TAA
		int m_CurrentJitterIndex = 0;
		Ref<Render::Texture> m_TAAResolveBuffer;
		Ref<Render::Texture> m_TAAHistoryBuffer;
		Ref<Render::Texture> m_TAAVelocityBuffer;
		Ref<Render::TextureView> m_TAAHistorySRVView;
		Ref<Render::TextureView> m_TAAHistoryUAVView;
		Ref<Render::TextureView> m_TAAResolveSRVView;
		Ref<Render::TextureView> m_TAAResolveUAVView;
		Ref<Render::RenderTargetView> m_TAAVelocityRTView;
		Ref<Render::TextureView> m_TAAVelocitySRVView;
		Ref<Render::Shader> m_TAAResolveComputeShader;
		Ref<Render::PipelineState> m_TAAResolvePSO;
		Mat44 m_PrevViewProjection = Mat44::Identity();
		Vector2f m_PrevJitter = Vector2f(0, 0);
	};
}