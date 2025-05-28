#pragma once

namespace vkr::Graphics
{
	class View;
	class ViewRenderer
	{
	public:
		ViewRenderer();
		~ViewRenderer();

		bool Init();

		void RenderView(View& view);

	private:
		void UpdateRtScene(View& view);
		void UpdateParticles(View& view);

		void DepthPrepass(View& view);

		void TraceRadiance(View& view);

		void ApplyUpscaling(View& view);
		void ApplyPostEffects(View& view);
		void FinalizeFrame(View& view);

	private:
		// example sub systems
		// UniquePtr<Environment> m_Environment;
		// UniquePtr<VegetationSystem> m_VegetationSystem;
		// UniquePtr<VfxSimulator> m_VfxSimulator;
	};
}