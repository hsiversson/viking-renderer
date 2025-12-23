#include "view.h"
#include "application/appsettings.h"
#include "render/device.h"
#include "render/nvdlss.h"

namespace 
{
	static uint32_t g_ViewIDCounter = 0;

	static float Halton(uint32_t index, uint32_t base)
	{
		float result = 0.0f;
		float f = 1.0f;
		uint32_t i = index;
		do
		{
			f /= static_cast<float>(base);
			result = result + f * static_cast<float>(index % base);
			index = static_cast<uint32_t>(floorf(static_cast<float>(index) / static_cast<float>(base)));
		} while (index > 0);
		return result;
	}
}

namespace vkr::Graphics
{

	bool TextureTarget::Update(uint32_t width, uint32_t height, const char* name)
	{
		return Update(2, Vector3u(width, height, 1), name);
	}

	bool TextureTarget::Update(Vector2u size, const char* name)
	{
		return Update(2, Vector3u(size.x, size.y, 1), name);
	}

	bool TextureTarget::Update(Vector3u size, const char* name /*= "Unnamed Texture"*/)
	{
		return Update(3, size, name);
	}

	bool TextureTarget::Update(uint32_t width, uint32_t height, uint32_t depth, const char* name /*= "Unnamed Texture"*/)
	{
		return Update(3, Vector3u(width, height, depth), name);
	}

	bool TextureTarget::Update(uint32_t dimension, Vector3u size, const char* name)
	{
		VKR_ASSERT(dimension == 2 || dimension == 3);
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
			changed |= textureDesc.m_Size != size;
			changed |= textureDesc.m_Format != m_Format;
		}

		if (!changed)
		{
			return false;
		}

		Render::TextureDesc textureDesc = {};
		textureDesc.m_Dimension = dimension == 2 ? Render::ResourceDimension::Texture2D : Render::ResourceDimension::Texture3D;
		textureDesc.m_Size = size;
		textureDesc.m_MipLevels = 1;
		textureDesc.m_Writable = m_IsWritable;
		textureDesc.m_AllowRenderTarget = m_IsRenderTarget;
		textureDesc.m_AllowDepthStencil = m_IsDepthStencil;
		textureDesc.m_Format = m_Format;
		textureDesc.m_ClearValue = m_ClearValue;
		textureDesc.m_NumSamples = m_NumSamples;
		textureDesc.m_Name = name;
		m_Texture = Render::GetDevice()->CreateTexture(textureDesc);
		VKR_ASSERT(m_Texture);

		Render::TextureViewDesc textureViewDesc = {};
		textureViewDesc.m_Format = m_Format;
		textureViewDesc.m_Mip = 0;
		textureViewDesc.m_NumSamples = m_NumSamples;
		textureViewDesc.m_Writable = false;
		m_TextureView = Render::GetDevice()->CreateTextureView(textureViewDesc, m_Texture);

		if (m_IsWritable)
		{
			Render::TextureViewDesc rwTextureViewDesc = {};
			rwTextureViewDesc.m_Format = m_Format;
			rwTextureViewDesc.m_Mip = 0;
			rwTextureViewDesc.m_NumSamples = m_NumSamples;
			rwTextureViewDesc.m_Writable = true;
			m_TextureViewRW = Render::GetDevice()->CreateTextureView(rwTextureViewDesc, m_Texture);
		}

		if (m_IsRenderTarget)
		{
			Render::RenderTargetViewDesc rtvDesc = {};
			rtvDesc.m_Format = m_Format;
			rtvDesc.m_NumSamples = m_NumSamples;
			m_RenderTarget = Render::GetDevice()->CreateRenderTargetView(rtvDesc, m_Texture);
		}
		else if (m_IsDepthStencil)
		{
			Render::DepthStencilViewDesc dsvDesc = {};
			dsvDesc.m_Format = m_Format;
			dsvDesc.m_NumSamples = m_NumSamples;
			m_DepthStencil = Render::GetDevice()->CreateDepthStencilView(dsvDesc, m_Texture);
		}
		return true;
	}

	View::View()
		: m_PrepareDataIndex(0)
		, m_RenderDataIndex(1)
		, m_OutputSize{}
		, m_RenderSize{}
		, m_OutputTarget(nullptr)
		, m_IsRendering(false)
		, m_IsPrimary(false)
	{
		m_ViewID = g_ViewIDCounter++;
		m_NvDLSS = MakeUnique<Render::NvDLSS>();
	}

	View::~View()
	{

	}

	void View::SetOutputSize(const Vector2u& size)
	{
		if (size != m_OutputSize)
		{
			m_OutputSize = size;
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

	void View::PrepareCameraConstants(CameraData& data)
	{
		Camera& cam = const_cast<Camera&>(GetCamera());
		Mat44 ProjectionNoJitter = cam.GetProjection();
		//Select a new jitter offset for TAA for this frame
		m_CurrentJitterFrame = (m_CurrentJitterFrame + 1) % 32;

		const Vector2f halton = Vector2f(Halton(m_CurrentJitterFrame, 2), Halton(m_CurrentJitterFrame, 3)) - 0.5f;
		data.CurrentJitter = (halton * 2.0f) / Vector2f(GetRenderSize());
		Mat44 Projection = ProjectionNoJitter;
		Projection[8] = data.CurrentJitter.x;
		Projection[9] = data.CurrentJitter.y;
		data.PrevJitter = m_PrevJitter;
		m_PrevJitter = data.CurrentJitter;

		Mat43 CamWorld = cam.GetWorldTransform();
		data.CameraWorldMatrix = CamWorld;
		data.PrevCameraWorldMatrix = m_PrevCameraWorld;
		m_PrevCameraWorld = data.CameraWorldMatrix;
		data.ViewMatrix = cam.GetView();
		data.InvViewMatrix = Inverse(data.ViewMatrix);
		data.ProjectionMatrix = Projection;
		data.InvProjectionMatrix = Inverse(data.ProjectionMatrix);
		data.ViewProjectionMatrix = data.ViewMatrix * data.ProjectionMatrix;
		data.InvViewProjectionMatrix = Inverse(data.ViewProjectionMatrix);
		data.PrevViewProjectionMatrix = m_PrevViewProjection;
		m_PrevViewProjection = data.ViewProjectionMatrix;
		data.ProjectionMatrixUnjittered = ProjectionNoJitter;
		data.PrevProjectionMatrixUnjittered = m_PrevProjectionUnjittered;
		m_PrevProjectionUnjittered = data.ProjectionMatrixUnjittered;
		data.InvProjectionMatrixUnjittered = Inverse(data.ProjectionMatrixUnjittered);
		data.PrevViewMatrix = m_PrevView;
		m_PrevView = data.ViewMatrix;
		data.ViewProjectionMatrixUnjittered = data.ViewMatrix * data.ProjectionMatrixUnjittered;
		data.InvViewProjectionMatrixUnjittered = Inverse(data.ViewProjectionMatrixUnjittered);
		data.PrevViewProjectionMatrixUnjittered = m_PrevViewProjectionUnjittered;
		m_PrevViewProjectionUnjittered = data.ViewProjectionMatrixUnjittered;
		data.AspectRatio = cam.GetAspectRatio();
		data.Near = cam.GetNearZ();
		data.Far = cam.GetFarZ();
		data.FOVDegrees = cam.GetFov();
	}

	void View::BeginRender()
	{
		if (ElapsedTimer::FrameIndex() > 10 && AppSettings::GetAppSettings()->GetGraphicsSettings().m_UpscalingType == UPSCALING_TYPE_DLSS)
		{
			//Prepare will set the correct render size based on the desired output size we have set for this view, based on the suggestion from DLSS API
			m_NvDLSS->Prepare(this);
		}
		else
		{
			//TAA, use same render size as output
			m_RenderSize = m_OutputSize;
		}
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

	vkr::Render::NvDLSS& View::GetDLSS()
	{
		VKR_ASSERT(m_NvDLSS);
		return *m_NvDLSS;
	}

}