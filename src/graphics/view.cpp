#include "view.h"
#include "render/device.h"

namespace vkr::Graphics
{
	bool TextureTarget::Update(uint32_t width, uint32_t height, const char* name)
	{
		VKR_ASSERT((m_IsRenderTarget && m_IsDepthStencil) == false);
		VKR_ASSERT((m_IsWritable && m_IsDepthStencil) == false);

		bool changed = false;
		changed |= m_Texture == nullptr;
		changed |= m_TextureView == nullptr;
		changed |= !m_TextureViewRW && m_IsWritable;
		changed |= !m_RenderTarget && m_IsRenderTarget;
		changed |= !m_DepthStencil && m_DepthStencil;

		if (m_Texture)
		{
			const Render::TextureDesc& textureDesc = m_Texture->m_TextureDesc;
			changed |= textureDesc.m_Size != Vector3u(width, height, 1);
			changed |= textureDesc.m_Format != m_Format;
		}

		if (!changed)
		{
			return false;
		}

		Render::TextureDesc textureDesc = {};
		textureDesc.m_Dimension = Render::ResourceDimension::Texture2D;
		textureDesc.m_Size = { width, height, 1 };
		textureDesc.m_MipLevels = 1;
		textureDesc.m_Writable = m_IsWritable;
		textureDesc.m_AllowRenderTarget = m_IsRenderTarget;
		textureDesc.m_AllowDepthStencil = m_IsDepthStencil;
		textureDesc.m_Format = m_Format;
		textureDesc.m_ClearValue = m_ClearValue;
		m_Texture = Render::GetDevice()->CreateTexture(textureDesc);

		m_TextureView = Render::GetDevice()->CreateTextureView({}, m_Texture);

		if (m_IsWritable)
		{
			m_TextureViewRW = Render::GetDevice()->CreateTextureView({0, true}, m_Texture);
		}

		if (m_IsRenderTarget)
		{
			m_RenderTarget = Render::GetDevice()->CreateRenderTargetView({}, m_Texture);
		}
		else if (m_IsDepthStencil)
		{
			m_DepthStencil = Render::GetDevice()->CreateDepthStencilView({}, m_Texture);
		}
		return true;
	}

	bool TextureTarget::Update(Vector2u size, const char* name)
	{
		return Update(size.x, size.y, name);
	}

	View::View()
		: m_PrepareDataIndex(0)
		, m_RenderDataIndex(1)
		, m_MaxRenderSize{}
		, m_CurrentRenderSize{}
		, m_OutputTarget(nullptr)
		, m_IsRendering(false)
		, m_IsPrimary(false)
	{

	}

	View::~View()
	{

	}

	void View::SetRenderSize(const Vector2u& size)
	{
		if (size != m_MaxRenderSize)
		{
			m_MaxRenderSize = size;

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

	ViewRenderTargets& View::GetRenderTargets()
	{
		return m_RenderTargets;
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