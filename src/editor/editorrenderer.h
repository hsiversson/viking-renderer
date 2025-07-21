#pragma once

#if ENABLE_EDITOR
namespace vkr::Render
{
	class PipelineState;
}

struct ImDrawList;
namespace vkr::Editor
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		bool Init();

		void Render();

	private:
		Ref<Render::PipelineState> m_SdrShader;
		Ref<Render::PipelineState> m_HdrShader;

		struct ImGuiVertex
		{
			Vector2f pos;
			Vector2f uv;
			uint32_t col;
		};

		struct RenderData
		{
			std::vector<ImDrawList*> m_DrawLists;
			std::vector<ImGuiVertex> m_Vertices;
			std::vector<uint16_t> m_Indices;

			RenderData() {}
			~RenderData() { Clear(); }
			void Clear();
		};
	};
}
#endif //ENABLE_EDITOR