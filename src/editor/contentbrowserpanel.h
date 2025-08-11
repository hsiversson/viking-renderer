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
			ContentDirectory m_ContentDirectory;
			std::filesystem::path m_Path;
			std::vector<std::filesystem::path> m_Files;
			std::vector<DirectoryEntry> m_ChildDirectories;
		};

		void DrawTopBar();
		void DrawDirectoryBrowser();
		void DrawDirectoryTreeNode(const DirectoryEntry& entry, ContentDirectory contentDirectory);
		void DrawContentArea();

		void GetEntriesForPath(ContentDirectory contentDirectory, const std::filesystem::path& currentEntryPath, DirectoryEntry& entry);
		void UpdateParentsAndCurrentEntry(const std::filesystem::path& currentEntryPath, DirectoryEntry& entry);

	private:
		DirectoryEntry m_EngineRootDirectory;
		DirectoryEntry m_ProjectRootDirectory;
		const DirectoryEntry* m_CurrentEntry;
	};
}
#endif //ENABLE_EDITOR