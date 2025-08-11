#pragma once

namespace vkr
{
	enum ContentDirectory : uint8_t
	{
		CONTENT_DIRECTORY_ENGINE,
		CONTENT_DIRECTORY_PROJECT,
	};

	namespace SystemPaths
	{
		bool Init();

		const std::filesystem::path& GetContentDirectory(ContentDirectory contentDirectory);
		std::filesystem::path GetInContentDirectory(ContentDirectory contentDirectory, const std::filesystem::path& pathToAppend);
		const std::filesystem::path& GetUserDirectory();

		const std::filesystem::path& GetExeDirectory();
		const std::filesystem::path& GetExePath();
	}
}