#pragma once
#include "viewrenderdata.h"
#include "camera.h"

namespace vkr::Render
{
	class DepthStencilView;
	class RenderTargetView;
	class Texture;
	class TextureView;
}

namespace vkr::Graphics
{
	struct TextureTarget
	{
		Ref<Render::Texture> m_Texture;
		Ref<Render::TextureView> m_TextureView;
		Ref<Render::TextureView> m_TextureViewRW;
		Ref<Render::RenderTargetView> m_RenderTarget;
		Ref<Render::DepthStencilView> m_DepthStencil;

		bool m_IsWritable = false;
		bool m_IsRenderTarget = false;
		bool m_IsDepthStencil = false;
		Render::Format m_Format = Render::FORMAT_UNKNOWN;
		Vector4f m_ClearValue = {0,0,0,0};

		bool Update(uint32_t width, uint32_t height, const char* name = "Unnamed Texture");
		bool Update(Vector2u size, const char* name = "Unnamed Texture");
	};

	struct ViewRenderTargets
	{
		TextureTarget m_DepthBuffer;
		TextureTarget m_DepthBuffer_Linear;
		TextureTarget m_Velocity;
		TextureTarget m_Normals;

		TextureTarget m_SceneBuffer_RenderSize;
		TextureTarget m_SceneBuffer_OutputSize;

		TextureTarget m_SceneHistory;
	};

	class View
	{
	public:
		View();
		~View();

		void SetRenderSize(const Vector2u& size);
		Vector2u GetRenderSize() { return m_MaxRenderSize; }

		void BeginPrepare();
		void EndPrepare();

		void BeginRender();
		void EndRender();

		void SetCamera(Camera& camera);
		const Camera& GetCamera() const;

		// We fill the render data in the preparation stage.
		ViewRenderData& GetPrepareData();

		// We consume the render data at render stage.
		const ViewRenderData& GetRenderData() const;
		ViewRenderData& GetMutableRenderData();

		void SetOutputTarget(Render::RenderTargetView* target) { m_OutputTarget = target; }
		Render::RenderTargetView* GetOutputTarget() const { return m_OutputTarget; }

		ViewRenderTargets& GetRenderTargets();

		void SetPrimary(bool value);

		bool IsPrimary() const;
		bool IsSecondary() const;

	private:
		bool InitTargets();

	private:
		std::array<ViewRenderData, 2> m_ViewRenderData;
		uint32_t m_PrepareDataIndex;
		uint32_t m_RenderDataIndex;
		Ref<Render::RenderTaskEvent> m_EndRenderEvent;

		Camera m_Camera;

		Vector2u m_MaxRenderSize;
		Vector2u m_CurrentRenderSize;

		Render::RenderTargetView* m_OutputTarget;
		ViewRenderTargets m_RenderTargets;

		//Global shaders

		bool m_IsRendering;
		bool m_IsPrimary;
	};

	struct PrepareViewContext
	{
		PrepareViewContext(View& view) : m_View(view)
		{
			m_View.BeginPrepare();
		}

		~PrepareViewContext()
		{
			m_View.EndPrepare();
		}

		View& m_View;
	};

	struct RenderViewContext
	{
		RenderViewContext(View& view) : m_View(view)
		{
			m_View.BeginRender();
		}

		~RenderViewContext()
		{
			m_View.EndRender();
		}

		View& m_View;
	};
}