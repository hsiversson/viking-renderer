#pragma once
#include "viewrenderdata.h"
#include "camera.h"

namespace vkr::Render
{
	class DepthStencilView;
	class NvDLSS;
	class RenderTargetView;
	class Texture;
	class TextureView;
}

namespace vkr::Graphics
{
	struct TextureTarget
	{
		Ref<Render::Texture> m_Texture;
		Ref<Render::TextureView> m_TextureView;
		Ref<Render::TextureView> m_TextureViewRW;
		Ref<Render::RenderTargetView> m_RenderTarget;
		Ref<Render::DepthStencilView> m_DepthStencil;

		bool m_IsWritable = false;
		bool m_IsRenderTarget = false;
		bool m_IsDepthStencil = false;
		Render::Format m_Format = Render::FORMAT_UNKNOWN;
		Vector4f m_ClearValue = {0,0,0,0};

		bool Update(uint32_t width, uint32_t height, const char* name = "Unnamed Texture");
		bool Update(Vector2u size, const char* name = "Unnamed Texture");
	};

	struct ViewRenderTargets
	{
		TextureTarget m_DepthBuffer;
		TextureTarget m_DepthBuffer_Linear;
		TextureTarget m_Velocity;
		TextureTarget m_NormalRoughness; // RGB: Normals, A: Roughness

		TextureTarget m_DiffuseAlbedo;
		TextureTarget m_SpecularAlbedo;

		TextureTarget m_SceneBuffer_RenderSize;
		TextureTarget m_SceneBuffer_OutputSize;

		TextureTarget m_SceneHistory;

		//Sky
		TextureTarget m_TransmittanceLUT;
		TextureTarget m_IrradianceLUT;
		TextureTarget m_ScatteringLUT;
		
		TextureTarget m_Exposure; // 1x1 with adapted exposure value
		Ref<Render::BufferView> m_ExposureHistogram;
	};

	class View
	{
	public:
		View();
		~View();

		void SetOutputSize(const Vector2u& size);
		Vector2u GetOutputSize() const { return m_OutputSize; }

		void SetRenderSize(const Vector2u& size) { m_RenderSize = size; }
		Vector2u GetRenderSize() const { return m_RenderSize; }

		void BeginPrepare();
		void EndPrepare();

		void BeginRender();
		void EndRender();

		void SetCamera(Camera& camera);
		const Camera& GetCamera() const;

		// We fill the render data in the preparation stage.
		ViewRenderData& GetPrepareData();
		void PrepareCameraConstants(CameraData& data);

		// We consume the render data at render stage.
		const ViewRenderData& GetRenderData() const;
		ViewRenderData& GetMutableRenderData();

		void SetOutputTarget(Render::RenderTargetView* target) { m_OutputTarget = target; }
		Render::RenderTargetView* GetOutputTarget() const { return m_OutputTarget; }

		ViewRenderTargets& GetRenderTargets();
		Render::NvDLSS& GetDLSS();

		void SetPrimary(bool value);

		bool IsPrimary() const;
		bool IsSecondary() const;

		uint32_t GetViewID() const { return m_ViewID; }

	private:
		std::array<ViewRenderData, 2> m_ViewRenderData;
		uint32_t m_PrepareDataIndex;
		uint32_t m_RenderDataIndex;
		Ref<Render::RenderTaskEvent> m_EndRenderEvent;
		UniquePtr<Render::NvDLSS> m_NvDLSS;

		Camera m_Camera;
		int m_CurrentJitterIndex = 0;
		Mat44 m_PrevCameraWorld = Mat44::Identity();
		Mat44 m_PrevView = Mat44::Identity();
		Mat44 m_PrevProjectionUnjittered = Mat44::Identity();
		Mat44 m_PrevViewProjection = Mat44::Identity();
		Mat44 m_PrevViewProjectionUnjittered = Mat44::Identity();
		Vector2f m_PrevJitter = Vector2f(0, 0);

		Vector2u m_OutputSize;
		Vector2u m_RenderSize;

		Render::RenderTargetView* m_OutputTarget;
		ViewRenderTargets m_RenderTargets;

		//Global shaders

		bool m_IsRendering;
		bool m_IsPrimary;

		uint32_t m_ViewID;
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