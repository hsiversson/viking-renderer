#include "view.h"

namespace vkr::Graphics
{

	View::View()
		: m_PrepareDataIndex(0)
		, m_RenderDataIndex(1)
		, m_IsRendering(false)
		, m_IsPrimary(false)
	{

	}

	View::~View()
	{

	}

	void View::BeginPrepare()
	{
		m_PrepareDataIndex = (m_PrepareDataIndex + 1) % m_ViewRenderData.size();
	}

	void View::EndPrepare()
	{
		// Make sure render from last frame has finished before we swap indices?
		if (m_IsRendering)
		{
			// stall
		}

		m_RenderDataIndex = m_PrepareDataIndex;
	}

	void View::BeginRender()
	{
		m_IsRendering = true;
	}

	void View::EndRender()
	{
		m_IsRendering = false;
	}

	void View::SetCamera(Camera& camera)
	{
		m_Camera = camera;
	}

	const Camera& View::GetCamera()
	{
		return m_Camera;
	}

	ViewRenderData& View::GetPrepareData()
	{
		return m_ViewRenderData[m_PrepareDataIndex];
	}

	const ViewRenderData& View::GetRenderData()
	{
		return m_ViewRenderData[m_RenderDataIndex];
	}

	void View::SetPrimary(bool value)
	{
		m_IsPrimary = value;
	}

	bool View::IsPrimary() const
	{
		return m_IsPrimary;
	}

	bool View::IsSecondary() const
	{
		return !m_IsPrimary;
	}

}