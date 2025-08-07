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
			checkNoEntry();
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

		//This will fail as downloaded release DLLs are not signed with the public trusted signatures recognized by Microsoft
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
		assert(m_pImpl->m_InterposerDLL);
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

}
