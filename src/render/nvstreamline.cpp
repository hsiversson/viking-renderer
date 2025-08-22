#include "nvstreamline.h"

#include "core/commandline.h"
#include "device.h"
#include "sl.h"
#include "sl_consts.h"
#include "sl_security.h"

namespace vkr::Render
{
	sl::Feature FeatureToSLFeature(NvStreamlineFeature feature)
	{
		switch (feature)
		{
		case DLSS:
			return sl::kFeatureDLSS;
		case DLSS_RR:
			return sl::kFeatureDLSS_RR;
		default:
			VKR_CHECK_NO_ENTRY();
			return 0;
		}
	}

	struct NvStreamline::PImpl
	{
		~PImpl() = default;

		typedef HRESULT(WINAPI* slCreateDXGIFactoryFn)(REFIID, void**);
		typedef HRESULT(WINAPI* slCreateDXGIFactory1Fn)(REFIID, void**);
		typedef HRESULT(WINAPI* slCreateDXGIFactory2Fn)(UINT, REFIID, void**);
		typedef HRESULT(WINAPI* slDXGIGetDebugInterface1Fn)(UINT, REFIID, void**);
		typedef HRESULT(WINAPI* slD3D12CreateDeviceFn)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);

		slCreateDXGIFactoryFn m_CreateDXGIFactoryFunc = nullptr;
		slCreateDXGIFactory1Fn m_CreateDXGIFactory1Func = nullptr;
		slCreateDXGIFactory2Fn m_CreateDXGIFactory2Func = nullptr;
		slD3D12CreateDeviceFn m_CreateD3D12DeviceFunc = nullptr;
		//PFun_slInit* slInit = nullptr;
		//PFun_slShutdown* slShutdown = nullptr;
		//PFun_slSetD3DDevice* slSetD3DDevice = nullptr;
		//PFun_slIsFeatureSupported* slIsFeatureSupported = nullptr;

		HMODULE m_InterposerDLL = nullptr;
	};

	void StreamlineLoggingCallback(sl::LogType type, const char* msg)
	{
		switch (type) {
		case sl::LogType::eInfo:
			VKR_LOG("Streamline: {}", msg);
			break;
		case sl::LogType::eWarn:
			VKR_WARNING("Streamline Warning: {}", msg);
			break;
		case sl::LogType::eError:
			VKR_ERROR("Streamline Error: {}", msg);
			break;
		}
	}

	bool NvStreamline::Init()
	{
		m_pImpl = MakeUnique<PImpl>();

		const bool isDebugging = CommandLine::Has("debug_streamline");

		//DLL and function load
		std::filesystem::path slInterposerPath = SystemPaths::GetExeDirectory() / "sl.interposer.dll";

		if (!sl::security::verifyEmbeddedSignature(slInterposerPath.c_str()))
		{
			return false;
 		}
		m_pImpl->m_InterposerDLL = LoadLibraryW(slInterposerPath.c_str());
		if (!m_pImpl->m_InterposerDLL)
		{
			return false;
		}

		//m_pImpl->slInit = (PFun_slInit*)GetProcAddress(m_pImpl->m_InterposerDLL, "slInit");
		//m_pImpl->slShutdown = (PFun_slShutdown*)GetProcAddress(m_pImpl->m_InterposerDLL, "slShutdown");
		//m_pImpl->slSetD3DDevice = (PFun_slSetD3DDevice*)GetProcAddress(m_pImpl->m_InterposerDLL, "slSetD3DDevice");
		//m_pImpl->slIsFeatureSupported = (PFun_slIsFeatureSupported*)GetProcAddress(m_pImpl->m_InterposerDLL, "slIsFeatureSupported");

		// Proceed to initialize Streamline
		sl::Preferences pref{};
		pref.showConsole = isDebugging;
		pref.logLevel = isDebugging ? sl::LogLevel::eDefault : sl::LogLevel::eOff;
		pref.logMessageCallback = StreamlineLoggingCallback;
		pref.applicationId = 100000000; // For development, although NGX may require a valid ID
		pref.engine = sl::EngineType::eCustom;
		pref.engineVersion = "1.0.0";
		pref.renderAPI = sl::RenderAPI::eD3D12;
		pref.projectId = 0;
		
		sl::Feature featuresToLoad[] = { 
			sl::kFeatureDLSS ,
			sl::kFeatureDLSS_RR
		};
		pref.featuresToLoad = featuresToLoad;
		pref.numFeaturesToLoad = _countof(featuresToLoad);
		
		pref.flags = {};
		pref.flags |= sl::PreferenceFlags::eDisableCLStateTracking;
 		pref.flags |= sl::PreferenceFlags::eUseDXGIFactoryProxy;

		sl::Result res = slInit(pref, sl::kSDKVersion);
		if (res != sl::Result::eOk) 
		{
			VKR_LOG("[NvStreamline] Init failed");
			FreeLibrary(m_pImpl->m_InterposerDLL);
			return false;
		}

		return true;
	}

	NvStreamline::NvStreamline() {}
	NvStreamline::~NvStreamline() {}

	bool NvStreamline::Shutdown()
	{
		if (!m_pImpl->m_InterposerDLL)
			return false;

		if (SL_FAILED(result, slShutdown()))
		{
			VKR_LOG("[NvStreamline] Failed to shutdown");
			return false;
		}

		FreeLibrary(m_pImpl->m_InterposerDLL);
		return true;
	}

	bool NvStreamline::SetDevice(const Device* device)
	{
		VKR_ASSERT(m_pImpl->m_InterposerDLL);
		if (SL_FAILED(result, slSetD3DDevice(device->GetD3DDevice())))
		{
			VKR_LOG("[NvStreamline] Failed to set D3D Device.");
			return false;
		}
		return true;
	}

	bool NvStreamline::IsFeatureAvailable(NvStreamlineFeature feature)
	{
		DXGI_ADAPTER_DESC adapterDesc = {};
		GetDevice()->GetDXGIAdapter()->GetDesc(&adapterDesc);

		sl::AdapterInfo adapterInfo{};
		adapterInfo.deviceLUID = (uint8_t*)&adapterDesc.AdapterLuid;
		adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);

		if (SL_FAILED(result, slIsFeatureSupported(FeatureToSLFeature(feature), adapterInfo)))
		{
			return false;
		}
		return true;
	}

	const sl::FrameToken* NvStreamline::GetFrameToken(uint32_t frameIndex) const
	{
		sl::FrameToken* token = nullptr;
		if (SL_FAILED(result, slGetNewFrameToken(token, &frameIndex)))
		{
			VKR_LOG("[NvStreamline] Failed to get frame token.");
		}
		return token;
	}

	void NvStreamline::Convert(sl::float2& aOut, const Vector2f& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
	}

	void NvStreamline::Convert(sl::float3& aOut, const Vector3f& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
		aOut.z = aVector.z;
	}

	void NvStreamline::Convert(sl::float4& aOut, const Vector4f& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
		aOut.z = aVector.z;
		aOut.w = aVector.w;
	}

	void NvStreamline::Convert(sl::float4x4& aOut, const Mat44& aMatrix)
	{
		Convert(aOut.row[0], Vector4f(aMatrix[0],aMatrix[1],aMatrix[2],aMatrix[3]));
		Convert(aOut.row[1], Vector4f(aMatrix[4], aMatrix[5], aMatrix[6], aMatrix[7]));
		Convert(aOut.row[2], Vector4f(aMatrix[8], aMatrix[9], aMatrix[10], aMatrix[11]));
		Convert(aOut.row[3], Vector4f(aMatrix[12], aMatrix[13], aMatrix[14], aMatrix[15]));
	}

	void NvStreamline::Convert(Vector2f& aOut, const sl::float2& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
	}

	void NvStreamline::Convert(Vector3f& aOut, const sl::float3& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
		aOut.z = aVector.z;
	}

	void NvStreamline::Convert(Vector4f& aOut, const sl::float4& aVector)
	{
		aOut.x = aVector.x;
		aOut.y = aVector.y;
		aOut.z = aVector.z;
		aOut.w = aVector.w;
	}

	void NvStreamline::Convert(Mat44& aOut, const sl::float4x4& aMatrix)
	{
		aOut[0] = aMatrix.row[0].x; aOut[1] = aMatrix.row[0].y; aOut[2] = aMatrix.row[0].z; aOut[3] = aMatrix.row[0].w;
		aOut[4] = aMatrix.row[1].x; aOut[5] = aMatrix.row[1].y; aOut[6] = aMatrix.row[1].z; aOut[7] = aMatrix.row[1].w;
		aOut[8] = aMatrix.row[2].x; aOut[9] = aMatrix.row[2].y; aOut[10] = aMatrix.row[2].z; aOut[11] = aMatrix.row[2].w;
		aOut[12] = aMatrix.row[3].x; aOut[13] = aMatrix.row[3].y; aOut[14] = aMatrix.row[3].z; aOut[15] = aMatrix.row[3].w;
	}


}
