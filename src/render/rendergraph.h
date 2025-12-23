#pragma once

namespace vkr::Render
{
	struct RenderGraphResource
	{

	};

	enum class RenderGraphPassType
	{
		Graphics,
		Compute
	};

	struct RenderGraphPass
	{
		RenderGraphPassType m_Type;
		std::string m_Name;
	};

	class RenderGraph
	{
	public:

		void CreateTexture();
		void CreateRenderTarget();
		void CreateDepthStencil();
		void CreateBuffer();
		void CreateSampler();

		void AddPass();

		void Compile();
		void Execute();

	private:
		void BuildDependencyTree();
	};
}