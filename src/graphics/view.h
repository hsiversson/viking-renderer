#pragma once
#include "viewrenderdata.h"
#include "camera.h"

namespace vkr::Render
{
	class ResourceDescriptor;
	class Texture;
}

namespace vkr::Graphics
{
	class View
	{
	public:
		View();
		~View();

		void SetRenderSize(const Vector2u& size);

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

		void SetOutputTarget(Ref<vkr::Render::ResourceDescriptor> outputdescriptor) { m_OutputDescriptor = outputdescriptor; }
		Ref<vkr::Render::ResourceDescriptor> GetOutputTarget() const { return m_OutputDescriptor; }

		Ref<vkr::Render::ResourceDescriptor> GetDepthBuffer() const { return m_DepthBufferView; }

		void SetPrimary(bool value);

		bool IsPrimary() const;
		bool IsSecondary() const;

	private:
		bool InitTargets();

	private:
		std::array<ViewRenderData, 2> m_ViewRenderData;
		uint32_t m_PrepareDataIndex;
		uint32_t m_RenderDataIndex;

		Camera m_Camera;
		Ref<vkr::Render::ResourceDescriptor> m_OutputDescriptor;

		Vector2u m_MaxRenderSize;
		Vector2u m_CurrentRenderSize;

		// encapsulate targets in a sub struct?
		Ref<Render::Texture> m_DepthBuffer;
		Ref<vkr::Render::ResourceDescriptor> m_DepthBufferView;

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