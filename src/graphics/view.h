#pragma once
#include "viewrenderdata.h"
#include "camera.h"

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

		void SetPrimary(bool value);

		bool IsPrimary() const;
		bool IsSecondary() const;

	private:
		std::array<ViewRenderData, 2> m_ViewRenderData;
		uint32_t m_PrepareDataIndex;
		uint32_t m_RenderDataIndex;

		Camera m_Camera;

		bool m_IsRendering;
		bool m_IsPrimary;
	};
}