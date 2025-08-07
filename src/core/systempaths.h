#pragma once

namespace vkr::SystemPaths
{
	bool Init();

	const std::filesystem::path& GetContentDirectory();
	const std::filesystem::path& GetUserDirectory();

	const std::filesystem::path& GetExeDirectory();
	const std::filesystem::path& GetExePath();
}