#pragma once
#include "utils/types.h"

namespace vkr::Render
{
	class Texture;
	class Sampler;
	class PipelineState;
}

namespace vkr::Graphics
{
	class Material
	{
	public:
		Material();
		~Material();

		void SetPipelineState(Ref<Render::PipelineState> pso) { m_PipelineState = pso; }
		Ref<Render::PipelineState> GetPipelineState() const { return m_PipelineState; }

	private:
		// Make this parameterized?
		std::vector<Ref<Render::Texture>> m_Textures;
		//std::vector<Ref<Render::Sampler>> m_Samplers; // This should probably not be stored, but rather requested from device to be able to handle dynamic mip biasing.

		// Should we have multiple?
		Ref<Render::PipelineState> m_PipelineState;
	};
};