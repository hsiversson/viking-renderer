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

		Render::DepthStencilView* GetDepthBuffer() const { return m_DepthBufferView.get(); }
		Render::TextureView* GetDepthBufferTextureView() const { return m_DepthTextureView.get(); }
		Render::Texture* GetDepthBufferTexture() const { return m_DepthBuffer.get(); }

		Render::Texture* GetSceneTexture() const { return m_SceneTexture.get(); }
		Render::TextureView* GetSceneTextureView() const { return m_SceneTextureView.get(); }
		Render::TextureView* GetSceneTextureViewRW() const { return m_SceneTextureViewRW.get(); }
		Render::RenderTargetView* GetSceneTextureRenderTarget() const { return m_SceneTextureRenderTarget.get(); }

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
		Render::RenderTargetView* m_OutputTarget;

		Vector2u m_MaxRenderSize;
		Vector2u m_CurrentRenderSize;

		// encapsulate targets in a sub struct?
		Ref<Render::Texture> m_DepthBuffer;
		Ref<Render::DepthStencilView> m_DepthBufferView;
		Ref<Render::TextureView> m_DepthTextureView;

		//Texture on which to render the scene pre-upscale
		Ref<Render::Texture> m_SceneTexture;
		Ref<Render::TextureView> m_SceneTextureView;
		Ref<Render::TextureView> m_SceneTextureViewRW;
		Ref<Render::RenderTargetView> m_SceneTextureRenderTarget;

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