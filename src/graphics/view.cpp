#include "view.h"
#include "render/device.h"

namespace vkr::Graphics
{

	View::View()
		: m_PrepareDataIndex(0)
		, m_RenderDataIndex(1)
		, m_MaxRenderSize{}
		, m_CurrentRenderSize{}
		, m_IsRendering(false)
		, m_IsPrimary(false)
	{

	}

	View::~View()
	{

	}

	void View::SetRenderSize(const Vector2u& size)
	{
		if (size.x != m_MaxRenderSize.x ||
			size.y != m_MaxRenderSize.y)
		{
			m_MaxRenderSize = size;
			InitTargets();
		}
	}

	void View::BeginPrepare()
	{
		m_PrepareDataIndex = (m_PrepareDataIndex + 1) % m_ViewRenderData.size();
		GetPrepareData().Clear();
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

	const Camera& View::GetCamera() const
	{
		return m_Camera;
	}

	ViewRenderData& View::GetPrepareData()
	{
		return m_ViewRenderData[m_PrepareDataIndex];
	}

	const ViewRenderData& View::GetRenderData() const
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

	bool View::InitTargets()
	{
		Render::TextureDesc depthStencilDesc;
		depthStencilDesc.m_Dimension = Render::ResourceDimension::Texture2D;
		depthStencilDesc.m_Size = { m_MaxRenderSize.x, m_MaxRenderSize.y, 0 };
		depthStencilDesc.m_CalculateMips = false;
		depthStencilDesc.m_AllowDepthStencil = true;
		depthStencilDesc.m_Format = Render::FORMAT_D32_FLOAT;
		m_DepthBuffer = Render::GetDevice().CreateTexture(depthStencilDesc);

		Render::ResourceDescriptorDesc dsvDesc = {};
		dsvDesc.Type = Render::ResourceDescriptorType::DSV;
		dsvDesc.TextureDesc.Mip = 0;
		m_DepthBufferView = Render::GetDevice().CreateDSView(m_DepthBuffer.get(), dsvDesc);

		return true;
	}

}