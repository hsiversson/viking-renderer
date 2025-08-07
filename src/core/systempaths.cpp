#include "systempaths.h"

#if IS_WINDOWS_PLATFORM
#	include <shlobj.h>
#	include <shobjidl.h>
#else
#	error Platform not supported!
#endif

namespace vkr::SystemPaths
{
	static std::filesystem::path g_ContentDirectory;
	static std::filesystem::path g_UserDirectory;
	static std::filesystem::path g_ExeDirectory;
	static std::filesystem::path g_ExePath;

	bool Init()
	{
#if IS_WINDOWS_PLATFORM
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(nullptr, exePath, MAX_PATH);

		wchar_t documentsPath[MAX_PATH];
		SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath);
#else
#	error Platform not supported!
#endif

		g_ExePath = std::filesystem::weakly_canonical(exePath);
		g_ExeDirectory = g_ExePath.parent_path();

		g_ContentDirectory = std::filesystem::weakly_canonical(g_ExeDirectory / ".." / ".." / "content");
		g_UserDirectory = std::filesystem::weakly_canonical(documentsPath);

		return true;
	}

	const std::filesystem::path& GetContentDirectory()
	{
		return g_ContentDirectory;
	}

	const std::filesystem::path& GetUserDirectory()
	{
		return g_UserDirectory;
	}

	const std::filesystem::path& GetExeDirectory()
	{
		return g_ExeDirectory;
	}

	const std::filesystem::path& GetExePath()
	{
		return g_ExePath;
	}


}