#include "nvdlss.h"

#include "application/appsettings.h"
#include "graphics/view.h"
#include "graphics/viewrenderdata.h"
#include "render/commandlist.h"
#include "render/context.h"
#include "render/nvstreamline.h"
#include "render/rendercommon.h"
#include "sl.h"
#include "sl_dlss.h"
#include "sl_matrix_helpers.h"

namespace vkr::Render
{
	struct NvDLSS::PImpl
	{
		~PImpl() = default;

		Vector2u m_MinRenderSize;
		Vector2u m_MaxRenderSize;
		Vector2u m_OptimalRenderSize;
		float m_OptimalSharpness;
		bool m_ResetDLSS = false;
		sl::DLSSOptions m_DLSSOptions = {};
	};

	NvDLSS::NvDLSS()
	{
		m_pImpl = MakeUnique<PImpl>();
	}

	void NvDLSS::Prepare(Graphics::View& view)
	{
		Vector2u dstSize = view.GetRenderData().m_OutputSize;

		sl::DLSSOptions options = {};
		options.outputWidth = dstSize.x;
		options.outputHeight = dstSize.y;
		options.sharpness = 0.5f;
		options.colorBuffersHDR = sl::eTrue;
		options.useAutoExposure = sl::eTrue;

		// Preset F is recommended by Nvidia for DLAA and Ultra Performance
		options.dlaaPreset = sl::DLSSPreset::ePresetF;
		options.ultraPerformancePreset = sl::DLSSPreset::ePresetF;

		// Preset E is recommended by Nvidia for Quality, Balanced and Performance
		options.qualityPreset = sl::DLSSPreset::ePresetE;
		options.balancedPreset = sl::DLSSPreset::ePresetE;
		options.performancePreset = sl::DLSSPreset::ePresetE;
		options.ultraQualityPreset = sl::DLSSPreset::ePresetE; // Ultra Quality is not really a setting to be used.

		//Must match the DLSSModes in appsettings.h
		static constexpr sl::DLSSMode modes[] = { sl::DLSSMode::eUltraPerformance, sl::DLSSMode::eMaxPerformance, sl::DLSSMode::eBalanced, sl::DLSSMode::eMaxQuality, sl::DLSSMode::eDLAA };
		options.mode = modes[AppSettings::GetAppSettings()->GetGraphicsSettings().m_DLSSMode];
		options.preExposure = 1.0f;

		if (m_pImpl->m_DLSSOptions.mode != options.mode ||
			m_pImpl->m_DLSSOptions.outputWidth != options.outputWidth ||
			m_pImpl->m_DLSSOptions.outputHeight != options.outputHeight)
		{
			slDLSSSetOptions(view.GetViewID(), options);

			sl::DLSSOptimalSettings optimalSettings = {};
			slDLSSGetOptimalSettings(options, optimalSettings);
			m_pImpl->m_MinRenderSize = Vector2u(optimalSettings.renderWidthMin, optimalSettings.renderHeightMin);
			m_pImpl->m_MaxRenderSize = Vector2u(optimalSettings.renderWidthMax, optimalSettings.renderHeightMax);
			m_pImpl->m_OptimalRenderSize = Vector2u(optimalSettings.optimalRenderWidth, optimalSettings.optimalRenderHeight);
			m_pImpl->m_OptimalSharpness = optimalSettings.optimalSharpness;

			if (options.mode == sl::DLSSMode::eOff)
				m_pImpl->m_OptimalRenderSize = dstSize;

			m_pImpl->m_DLSSOptions = options;
			m_pImpl->m_ResetDLSS = true;
			view.SetRenderSize(m_pImpl->m_OptimalRenderSize);
		}
	}

	void NvDLSS::Upscale(Graphics::View& view, Render::Context* ctx)
	{
// 		if (m_pImpl->m_DLSSOptions.mode == sl::DLSSMode::eOff)
// 		{
// 			aTarget = aSource;
// 			return;
// 		}
		const Graphics::ViewRenderData& renderData =  view.GetRenderData();
		Graphics::ViewRenderTargets& renderTargets = view.GetRenderTargets();
		const Graphics::CameraData& renderCameraConstants = renderData.m_CameraData;
		const Vector2u srcSize = renderData.m_RenderSize;
		const Vector2u dstSize = renderData.m_OutputSize;
		sl::ViewportHandle viewportHandle = view.GetViewID();

		const NvStreamline* streamline = GetDevice()->GetNvStreamline();
		const sl::FrameToken& frameToken = *streamline->GetFrameToken(renderData.m_FrameIndex);

		sl::Constants constants = {};
		NvStreamline::Convert(constants.cameraPos, Vector3f(renderCameraConstants.CameraWorldMatrix[9], renderCameraConstants.CameraWorldMatrix[10], renderCameraConstants.CameraWorldMatrix[11]));
		constants.cameraAspectRatio = renderCameraConstants.AspectRatio;
		constants.cameraNear = renderCameraConstants.Near;
		constants.cameraFar = renderCameraConstants.Far;
		constants.cameraFOV =  vkr::DegToRad(renderCameraConstants.FOVDegrees);
		NvStreamline::Convert(constants.cameraRight, Vector3f(renderCameraConstants.CameraWorldMatrix[0], renderCameraConstants.CameraWorldMatrix[1], renderCameraConstants.CameraWorldMatrix[2]));
		NvStreamline::Convert(constants.cameraUp, Vector3f(renderCameraConstants.CameraWorldMatrix[3], renderCameraConstants.CameraWorldMatrix[4], renderCameraConstants.CameraWorldMatrix[5]));
		NvStreamline::Convert(constants.cameraFwd, Vector3f(renderCameraConstants.CameraWorldMatrix[6], renderCameraConstants.CameraWorldMatrix[7], renderCameraConstants.CameraWorldMatrix[8]));
		constants.cameraMotionIncluded = sl::eTrue;
		constants.motionVectorsJittered = sl::eFalse;
		constants.cameraPinholeOffset = { 0.0f, 0.0f };
		
		NvStreamline::Convert(constants.cameraViewToClip, renderCameraConstants.ViewProjectionMatrixUnjittered);
		NvStreamline::Convert(constants.clipToCameraView, renderCameraConstants.InvViewProjectionMatrixUnjittered);

		sl::float4x4 slCameraToWorldLastFrame;
		sl::float4x4 cameraToWorld;
		NvStreamline::Convert(slCameraToWorldLastFrame, renderCameraConstants.PrevCameraWorldMatrix);
		NvStreamline::Convert(cameraToWorld, renderCameraConstants.CameraWorldMatrix);

		sl::float4x4 cameraViewToPrevCameraView;
		calcCameraToPrevCamera(cameraViewToPrevCameraView, cameraToWorld, slCameraToWorldLastFrame);

		sl::float4x4 clipToPrevCameraView;
		matrixMul(clipToPrevCameraView, constants.clipToCameraView, cameraViewToPrevCameraView);

		sl::float4x4 cameraToClipLastFrame;
		NvStreamline::Convert(cameraToClipLastFrame, renderCameraConstants.PrevViewProjectionMatrixUnjittered);
		matrixMul(constants.clipToPrevClip, clipToPrevCameraView, cameraToClipLastFrame);
		matrixFullInvert(constants.prevClipToClip, constants.clipToPrevClip);

		recalculateCameraMatrices(constants);

		constants.mvecScale.x = 0.5f;
		constants.mvecScale.y = -0.5f;
		constants.jitterOffset.x = renderCameraConstants.CurrentJitter.x;
		constants.jitterOffset.y = renderCameraConstants.CurrentJitter.y;

		constants.reset = sl::Boolean(m_pImpl->m_ResetDLSS);
		constants.motionVectors3D = sl::eFalse;
		constants.depthInverted = sl::eTrue;

		slSetConstants(constants, frameToken, viewportHandle);

		const D3D12_RESOURCE_STATES readState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
		const D3D12_RESOURCE_STATES writeState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		
		sl::Resource colorIn = { sl::ResourceType::eTex2d, renderTargets.m_SceneBuffer_RenderSize.m_Texture->GetD3DResource(), readState };
		sl::Resource colorOut = { sl::ResourceType::eTex2d, renderTargets.m_SceneBuffer_OutputSize.m_Texture->GetD3DResource(), writeState };
		sl::Resource depth = { sl::ResourceType::eTex2d, renderTargets.m_DepthBuffer.m_Texture->GetD3DResource(), readState };
		sl::Resource mvec = { sl::ResourceType::eTex2d, renderTargets.m_Velocity.m_Texture->GetD3DResource(), readState };
		//sl::Resource exposure	= { sl::ResourceType::eTex2d, renderTargets.mAverageExposure.mResource ? renderTargets.mAverageExposure.mResource->mD3D12Resource : nullptr, readState };
		//sl::Resource bias		= { sl::ResourceType::eTex2d, nullptr,													readState };

		const sl::Extent renderVp = { 0, 0, srcSize.x, srcSize.y }; //Do we have viewport offsets?
		const sl::Extent targetVp = { 0, 0, dstSize.x, dstSize.y };
		//const sl::Extent onePxVp = { 0, 0, 1, 1 };

		sl::ResourceTag colorInTag = sl::ResourceTag{ &colorIn,	sl::kBufferTypeScalingInputColor,	 sl::ResourceLifecycle::eValidUntilEvaluate, &renderVp };
		sl::ResourceTag colorOutTag = sl::ResourceTag{ &colorOut,	sl::kBufferTypeScalingOutputColor,	 sl::ResourceLifecycle::eValidUntilEvaluate, &targetVp };
		sl::ResourceTag depthTag = sl::ResourceTag{ &depth,		sl::kBufferTypeDepth,				 sl::ResourceLifecycle::eValidUntilEvaluate, &renderVp };
		sl::ResourceTag mvTag = sl::ResourceTag{ &mvec,		sl::kBufferTypeMotionVectors,		 sl::ResourceLifecycle::eValidUntilEvaluate, &renderVp };
		//sl::ResourceTag exposureTag = sl::ResourceTag{ &exposure,	sl::kBufferTypeExposure,			 sl::ResourceLifecycle::eValidUntilEvaluate, &onePxVp  };
		//sl::ResourceTag biasTag		= sl::ResourceTag{ &bias,		sl::kBufferTypeBiasCurrentColorHint, sl::ResourceLifecycle::eValidUntilEvaluate, &renderVp };

		//KT_HybridArray<sl::ResourceTag, 8> globalTags;
		//resources.Add(colorInTag);
		//resources.Add(colorOutTag);
		//resources.Add(depthTag);
		//resources.Add(mvTag);
		//resources.Add(exposureTag);
		//resources.Add(biasTag);
		//slSetTag(viewportHandle, resources.GetBuffer(), resources.Count(), cmdBuffer->GetD3D12CommandList());

		std::vector<const sl::BaseStructure*> evalInputs;
		evalInputs.reserve(8);
		evalInputs.push_back(&viewportHandle);
		evalInputs.push_back(&colorInTag);
		evalInputs.push_back(&colorOutTag);
		evalInputs.push_back(&depthTag);
		evalInputs.push_back(&mvTag);
		//evalInputs.Add(&exposureTag);
		//evalInputs.Add(&biasTag);
		
		if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, frameToken, evalInputs.data(), evalInputs.size(), ctx->GetCommandList()->GetD3DCommandList())))
		{
			VKR_ERROR("[DLSS] slEvaluateFeature failed.");
		}

		m_pImpl->m_ResetDLSS = false;
	}

	NvDLSS::~NvDLSS()
	{

	}
	

}