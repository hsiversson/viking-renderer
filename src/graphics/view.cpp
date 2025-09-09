#include "view.h"
#include "application/appsettings.h"
#include "render/device.h"
#include "render/nvdlss.h"

namespace 
{
	static uint32_t g_ViewIDCounter = 0;

	static constexpr vkr::Vector2f JitterHaltonSequence[] = {
	{0.5,0.333333},
	{0.25,0.666667},
	{0.750000, 0.111111},
	{0.125000, 0.444444},
	{0.625000, 0.777778},
	{0.375000, 0.222222},
	{0.875000, 0.555556},
	{0.062500, 0.888889},
	{0.562500, 0.037037},
	{0.312500, 0.370370},
	{0.812500, 0.703704},
	{0.187500, 0.148148},
	{0.687500, 0.481481},
	{0.437500, 0.814815},
	{0.937500, 0.259259},
	{ 0.031250, 0.592593 }
	};

}

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
		VKR_ASSERT(m_Texture);

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
		int jitterIdx = m_CurrentJitterIndex++;
		m_CurrentJitterIndex = m_CurrentJitterIndex % 16;
		data.CurrentJitter = (JitterHaltonSequence[jitterIdx] - 0.5f) / Vector2f(GetRenderSize()) * 2.0f;
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
		if (ElapsedTimer::FrameIndex() > 10 && AppSettings::GetAppSettings()->GetGraphicsSettings().m_AAMethod == DLSS)
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