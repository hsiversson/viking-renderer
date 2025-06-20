#pragma once
#include "viewrenderdata.h"
#include "camera.h"

#include "render/resourcedescriptor.h"

namespace vkr::Graphics
{
	class View
	{
	public:
		View();
		~View();

		void BeginPrepare();
		void EndPrepare();

		void BeginRender();
		void EndRender();

		void SetCamera(Camera& camera);
		const Camera& GetCamera();

		// We fill the render data in the preparation stage.
		ViewRenderData& GetPrepareData();

		// We consume the render data at render stage.
		const ViewRenderData& GetRenderData();

		void SetOutputTarget(Ref<vkr::Render::ResourceDescriptor> outputdescriptor) { m_OutputDescriptor = outputdescriptor; }
		Ref<vkr::Render::ResourceDescriptor> GetOutputTarget() { return m_OutputDescriptor; }

		void SetDepthStencil(Ref<Render::ResourceDescriptor> dsDescriptor) { m_DSDescriptor = dsDescriptor; }
		Ref<vkr::Render::ResourceDescriptor> GetDepthStencil() { return m_DSDescriptor; }

		void SetPrimary(bool value);

		bool IsPrimary() const;
		bool IsSecondary() const;

	private:
		std::array<ViewRenderData, 2> m_ViewRenderData;
		uint32_t m_PrepareDataIndex;
		uint32_t m_RenderDataIndex;

		Camera m_Camera;
		Ref<vkr::Render::ResourceDescriptor> m_OutputDescriptor;
		Ref<vkr::Render::ResourceDescriptor> m_DSDescriptor;

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