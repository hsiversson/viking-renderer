#pragma once

#include "utils/types.h"

namespace vkr::Render
{
	class Device;
}

namespace vkr::Graphics
{
	class View;
	class ViewRenderer
	{
	public:
		ViewRenderer();
		~ViewRenderer();

		bool Init(Ref<vkr::Render::Device> device);

		void RenderView(View& view);

	private:
		void ForwardPass(View& view);
		void UpdateRtScene(View& view);
		void UpdateParticles(View& view);

		void DepthPrepass(View& view);

		void TraceRadiance(View& view);

		void ApplyUpscaling(View& view);
		void ApplyPostEffects(View& view);
		void FinalizeFrame(View& view);

	private:
		Ref<vkr::Render::Device> m_Device;
		// example sub systems
		// UniquePtr<Environment> m_Environment;
		// UniquePtr<VegetationSystem> m_VegetationSystem;
		// UniquePtr<VfxSimulator> m_VfxSimulator;
	};
}