#pragma once
#include "core/types.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class TextureView;
	class Sampler;
	class PipelineState;
	class Shader;
}

namespace vkr::Graphics
{
	struct MaterialDesc
	{
		std::vector<std::filesystem::path> m_TexturePaths;
		bool m_FrontCounterClockwise;
		bool m_TwoSided;
	};

	class Material
	{
	public:
		Material();
		~Material();

		bool Init(const MaterialDesc& desc);

		Ref<Render::PipelineState> GetDepthPipelineState(const Render::VertexLayout& vertexLayout);
		Ref<Render::PipelineState> GetDefaultPipelineState(const Render::VertexLayout& vertexLayout);
		Render::TextureView* GetTexture(uint32_t index) const;

	private:
		Ref<Render::PipelineState> GetOrCreatePSO(const Render::VertexLayout& vertexLayout, bool depthOnly);

	private:
		Ref<Render::Shader> m_PixelShader;

		using CachedPSOs = std::unordered_map<Render::VertexLayout, Ref<Render::PipelineState>>;
		CachedPSOs m_DefaultPSOs;
		CachedPSOs m_DepthOnlyPSOs;

		// Make this parameterized?
		std::vector<Ref<Render::TextureView>> m_Textures;

		bool m_FrontCounterClockwise;
		bool m_TwoSided;

		//std::vector<Ref<Render::Sampler>> m_Samplers; // This should probably not be stored, but rather requested from device to be able to handle dynamic mip biasing.
	};
};