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
		// Make sure render from last frame has finished before we swap indices?
		Render::QueueGraphicsTask([](){})->WaitForEvent();
		m_PrepareDataIndex = (m_PrepareDataIndex + 1) % m_ViewRenderData.size();
		GetPrepareData().Clear();
	}

	void View::EndPrepare()
	{
		m_RenderDataIndex = m_PrepareDataIndex;
	}

	void View::BeginRender()
	{
		m_IsRendering = true;
	}

	void View::EndRender()
	{
		m_IsRendering = false;
		m_EndRenderEvent = Render::QueueGraphicsTask([]() {});
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

	ViewRenderData& View::GetMutableRenderData()
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
		Render::TextureDesc depthStencilDesc = {};
		depthStencilDesc.m_Dimension = Render::ResourceDimension::Texture2D;
		depthStencilDesc.m_Size = { m_MaxRenderSize.x, m_MaxRenderSize.y, 0 };
		depthStencilDesc.m_MipLevels = 1;
		depthStencilDesc.m_AllowDepthStencil = true;
		depthStencilDesc.m_Format = Render::FORMAT_D32_FLOAT;
		m_DepthBuffer = Render::GetDevice()->CreateTexture(depthStencilDesc);

		m_DepthTextureView = Render::GetDevice()->CreateTextureView({}, m_DepthBuffer);

		Render::DepthStencilViewDesc dsvDesc;
		dsvDesc.m_Mip = 0;
		m_DepthBufferView = Render::GetDevice()->CreateDepthStencilView(dsvDesc, m_DepthBuffer);

		//Create scene texture, later on this one will be upscaled into the final output texture
		Render::TextureDesc sceneTextureDesc;
		sceneTextureDesc.m_AllowDepthStencil = false;
		sceneTextureDesc.m_AllowRenderTarget = true;
		sceneTextureDesc.m_Dimension = Render::ResourceDimension::Texture2D;
		sceneTextureDesc.m_ArraySize = 1;
		sceneTextureDesc.m_Format = Render::FORMAT_RGB10A2_UNORM; //TODO: Is there a way to know which format we should be creating this in?? This should match final output texture format
		sceneTextureDesc.m_MipLevels = 1;
		sceneTextureDesc.m_Size = { m_MaxRenderSize.x, m_MaxRenderSize.y, 0 };
		sceneTextureDesc.m_Writable = true;
		m_SceneTexture = Render::GetDevice()->CreateTexture(sceneTextureDesc);
		m_SceneTextureView = Render::GetDevice()->CreateTextureView({}, m_SceneTexture);
		m_SceneTextureViewRW = Render::GetDevice()->CreateTextureView({0, true}, m_SceneTexture);
		m_SceneTextureRenderTarget = Render::GetDevice()->CreateRenderTargetView({}, m_SceneTexture);
		return true;
	}

}