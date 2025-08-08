#pragma once

#if ENABLE_EDITOR
#include "panel.h"

namespace vkr::Editor
{
	class ContentBrowserPanel final : public Panel
	{
	public:
		ContentBrowserPanel();
		~ContentBrowserPanel() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

		struct DirectoryEntry
		{
			const DirectoryEntry* m_Parent;
			std::filesystem::path m_Path;
			std::vector<std::filesystem::path> m_Files;
			std::vector<DirectoryEntry> m_ChildDirectories;
		};

		void DrawTopBar();
		void DrawDirectoryBrowser();
		void DrawDirectoryTreeNode(const DirectoryEntry& entry);
		void DrawContentArea();

		void GetEntriesForPath(DirectoryEntry& entry, const std::filesystem::path& currentEntryPath);
		void UpdateParentsAndCurrentEntry(DirectoryEntry& entry, const std::filesystem::path& currentEntryPath);

	private:
		DirectoryEntry m_RootDirectory;
		const DirectoryEntry* m_CurrentEntry;
	};
}
#endif //ENABLE_EDITOR