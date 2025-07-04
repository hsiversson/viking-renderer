#pragma once
#include "core/types.h"

namespace vkr::Render
{
	class TextureView;
	class Sampler;
	class PipelineState;
}

namespace vkr::Graphics
{
	struct MaterialDesc
	{

	};

	class Material
	{
	public:
		Material();
		~Material();

		void SetDepthPipelineState(Ref<Render::PipelineState> pso) { m_DepthPipelineState = pso; }
		Ref<Render::PipelineState> GetDepthPipelineState() const { return m_DepthPipelineState; }
		void SetDefaultPipelineState(Ref<Render::PipelineState> pso) { m_DefaultPipelineState = pso; }
		Ref<Render::PipelineState> GetDefaultPipelineState() const { return m_DefaultPipelineState; }

		void AddTexture(const Ref<Render::TextureView>& tex);
		Render::TextureView* GetTexture() const;

	private:
		// Make this parameterized?
		std::vector<Ref<Render::TextureView>> m_Textures;
		//std::vector<Ref<Render::Sampler>> m_Samplers; // This should probably not be stored, but rather requested from device to be able to handle dynamic mip biasing.

		// In the future we would like a full fledged PSO management system
		Ref<Render::PipelineState> m_DepthPipelineState;
		Ref<Render::PipelineState> m_DefaultPipelineState;
	};
};