#include "systempaths.h"

#if IS_WINDOWS_PLATFORM
#	include <shlobj.h>
#	include <shobjidl.h>
#else
#	error Platform not supported!
#endif

namespace vkr::SystemPaths
{
		static std::filesystem::path g_EngineContentDirectory;
		static std::filesystem::path g_ProjectContentDirectory;
		static std::filesystem::path g_UserDirectory;
		static std::filesystem::path g_ExeDirectory;
		static std::filesystem::path g_ExePath;

		bool Init(const std::filesystem::path& exePath, const std::filesystem::path& projectContentDirectory)
		{
#if IS_WINDOWS_PLATFORM
			wchar_t documentsPath[MAX_PATH];
			SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, documentsPath);
#else
#	error Platform not supported!
#endif

			g_ExePath = exePath;
			g_ExeDirectory = exePath.parent_path();

			g_EngineContentDirectory = std::filesystem::weakly_canonical(g_ExeDirectory / ".." / ".." / "content");
			g_ProjectContentDirectory = projectContentDirectory;
			g_UserDirectory = std::filesystem::weakly_canonical(documentsPath);

			return true;
		}

		const std::filesystem::path& GetContentDirectory(ContentDirectory contentDirectory)
		{
			if (contentDirectory == CONTENT_DIRECTORY_PROJECT)
			{
				return g_ProjectContentDirectory;
			}
			else
			{
				return g_EngineContentDirectory;
			}
		}

		std::filesystem::path GetInContentDirectory(ContentDirectory contentDirectory, const std::filesystem::path& pathToAppend)
		{
			return GetContentDirectory(contentDirectory) / pathToAppend;
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