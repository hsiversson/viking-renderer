#pragma once

#if ENABLE_EDITOR
namespace vkr::Render
{
	class PipelineState;
	class RenderTaskEvent;
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
		void RenderTask(const uint32_t renderDataIndex);

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
			Vector2f m_ViewportOffset;
			Vector2f m_ViewportSize;
			Vector2f m_ViewportScale;
			Ref<Render::RenderTaskEvent> m_Event;

			RenderData() {}
			~RenderData() { Clear(); }
			void Clear();
		};
		std::array<RenderData, 3> m_RenderData;
		uint32_t m_CurrentRenderDataIndex;
	};
}
#endif //ENABLE_EDITOR