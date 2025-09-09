
#pragma once

namespace vkr::Render
{
	class PipelineState;
	class Shader;
}

namespace vkr::Graphics
{
	class View;
	class Sky
	{
	public:
		Sky() = default;
		~Sky() = default;

		bool Init();

		void PrepareView(View* view);

	private:
	};

	class SkyRenderer
	{
	public:
		SkyRenderer() = default;
		~SkyRenderer() = default;

		bool Init();

		void ComputeLuts(View* view);

	private:
		Ref<Render::Shader> m_SkyTransmittanceLUTComputeShader;
		Ref<Render::PipelineState> m_SkyTransmittanceLUTPSO;
		Ref<Render::PipelineState> m_SkyIrradianceLUTPSO;
	};
}