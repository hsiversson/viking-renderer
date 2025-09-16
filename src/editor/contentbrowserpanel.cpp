#include "contentbrowserpanel.h"

#if ENABLE_EDITOR
#include "editor.h"

namespace vkr::Editor
{
	ContentBrowserPanel::ContentBrowserPanel()
		: Panel("Content Browser")
		, m_CurrentEntry(nullptr)
	{
	}

	ContentBrowserPanel::~ContentBrowserPanel()
	{
	}

	void ContentBrowserPanel::OnUpdate()
	{
		// Scan for changes in directories
		std::filesystem::path currentEntryPath;
		const DirectoryEntry* currentEntry = m_CurrentEntry;
		while (currentEntry)
		{
			if (!std::filesystem::exists(currentEntry->m_Path))
				currentEntry = currentEntry->m_Parent;
			else
			{
				currentEntryPath = currentEntry->m_Path;
				break;
			}
		}

		if (!currentEntry)
			currentEntryPath = SystemPaths::GetContentDirectory(CONTENT_DIRECTORY_PROJECT);

		std::filesystem::path p = SystemPaths::GetContentDirectory(CONTENT_DIRECTORY_PROJECT);
		m_ProjectRootDirectory.m_Parent = nullptr;
		GetEntriesForPath(CONTENT_DIRECTORY_PROJECT, p, m_ProjectRootDirectory);
		UpdateParentsAndCurrentEntry(currentEntryPath, m_ProjectRootDirectory);

		p = SystemPaths::GetContentDirectory(CONTENT_DIRECTORY_ENGINE);
		m_EngineRootDirectory.m_Parent = nullptr;
		GetEntriesForPath(CONTENT_DIRECTORY_ENGINE, p, m_EngineRootDirectory);
		UpdateParentsAndCurrentEntry(currentEntryPath, m_EngineRootDirectory);
	}

	void ContentBrowserPanel::OnDraw()
	{
		if (!m_CurrentEntry)
			return;

		DrawTopBar();
		if (ImGui::BeginTable("#contentBrowserLayout", 2, ImGuiTableFlags_BordersInnerV))
		{
			ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, 256 + ImGui::GetStyle().ItemSpacing.x);

			ImGui::TableNextColumn();
			DrawDirectoryBrowser();
			ImGui::TableNextColumn();
			DrawContentArea();
			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawTopBar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("##contentBrowserTopBar", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.243f, 0.62f, 0.047f, 1));
		if (ImGui::Button("Import", ImVec2(256 + ImGui::GetStyle().ItemSpacing.x, 38)))
		{
			// TODO: Handle import dialog
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();

		Icons* icons = Editor::Manager::Get()->GetIcons();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

		const bool isAtBase = m_CurrentEntry->m_Parent == nullptr;
		if (isAtBase)
		{
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);
		}

		if (ImGui::ImageButton("##contentBrowserNavLeft", (ImTextureID)icons->GetIcon(EDITOR_ICON_LEFT_ARROW_WHITE).m_Texture.get(), ImVec2(32, 32)) && !isAtBase)
			m_CurrentEntry = m_CurrentEntry->m_Parent;

		if (isAtBase)
		{
			ImGui::PopItemFlag();
			ImGui::PopStyleVar();
		}

		ImGui::SameLine(0.0f, 0.0f);

		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.25f);
		ImGui::ImageButton("##contentBrowserNavRight", (ImTextureID)icons->GetIcon(EDITOR_ICON_RIGHT_ARROW_WHITE).m_Texture.get(), ImVec2(32, 32));
		ImGui::PopStyleVar();
		ImGui::PopItemFlag();

		ImGui::SameLine(0.0f, 4.0f);
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
		ImGui::ImageButton("##contentBrowserNavFolder", (ImTextureID)icons->GetIcon(EDITOR_ICON_FOLDER).m_Texture.get(), ImVec2(32, 32));
		ImGui::PopItemFlag();

		ImGui::SameLine();

		std::vector<std::pair<std::string, const DirectoryEntry*>> folders;
		const DirectoryEntry* currentEntry = m_CurrentEntry;
		while (currentEntry)
		{
			folders.push_back(std::pair(currentEntry->m_Path.filename().string(), currentEntry));
			currentEntry = currentEntry->m_Parent;
		}

		ImFont font = *ImGui::GetFont();
		font.Scale = 1.25f;
		ImGui::PushFont(&font);

		const float originalCursorPosY = ImGui::GetCursorPosY();
		for (int32_t i = folders.size() - 1; i >= 0; --i)
		{
			const DirectoryEntry& entry = *(folders[i].second);
			const std::string& entryName = folders[i].first;

			const char* rootDirName = (entry.m_ContentDirectory == CONTENT_DIRECTORY_PROJECT) ? "Project" : "Engine";
			const std::string& folderName = (entry.m_Parent == nullptr) ? rootDirName : entryName;

			if (i != folders.size() - 1)
			{
				ImGui::SameLine();
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 11);
				ImGui::Image((ImTextureID)icons->GetIcon(EDITOR_ICON_PLAY_WHITE).m_Texture.get(), ImVec2(16, 16));
				ImGui::PopItemFlag();
				ImGui::SameLine();
			}
			ImGui::SetCursorPosY(originalCursorPosY);
			ImVec2 textSize = ImGui::CalcTextSize(folderName.c_str());
			if (ImGui::Button(folderName.c_str(), ImVec2(textSize.x + ImGui::GetStyle().ItemSpacing.x, 38)))
				m_CurrentEntry = folders[i].second;
		}

		ImGui::PopFont();
		ImGui::PopStyleColor(2);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawDirectoryBrowser()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("##contentBrowserDirectoryBrowser", ImVec2(0, 0), ImGuiChildFlags_Borders);
		//if (ImGui::Button("Game Dir", ImVec2(128.0f, 32.0f)))
		//{
		//	mCurrentEntry = nullptr;
		//	mIsEngineBase = false;
		//	OnUpdate();
		//}
		//ImGui::SameLine();
		//if (ImGui::Button("Engine Dir", ImVec2(128.0f, 32.0f)))
		//{
		//	mCurrentEntry = nullptr;
		//	mIsEngineBase = true;
		//	OnUpdate();
		//}
		DrawDirectoryTreeNode(m_EngineRootDirectory, CONTENT_DIRECTORY_ENGINE);
		DrawDirectoryTreeNode(m_ProjectRootDirectory, CONTENT_DIRECTORY_PROJECT);

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawDirectoryTreeNode(const DirectoryEntry& entry, ContentDirectory contentDirectory)
	{
		Icons* icons = Editor::Manager::Get()->GetIcons();
		float fontSize = ImGui::GetFont()->FontSize;
		ImGui::Image((ImTextureID)icons->GetIcon(EDITOR_ICON_FOLDER).m_Texture.get(), ImVec2(fontSize, fontSize));
		ImGui::SameLine();

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		if (entry.m_ChildDirectories.empty())
			flags |= ImGuiTreeNodeFlags_Leaf;

		const char* rootDirName = (contentDirectory == CONTENT_DIRECTORY_PROJECT) ? "Project" : "Engine";
		const std::string& folderName = (entry.m_Parent == nullptr) ? rootDirName : entry.m_Path.filename().string();
		bool isOpen = ImGui::TreeNodeEx(folderName.c_str(), flags);

		if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			m_CurrentEntry = &entry;

		if (isOpen)
		{
			for (uint32_t i = 0; i < entry.m_ChildDirectories.size(); ++i)
				DrawDirectoryTreeNode(entry.m_ChildDirectories[i], contentDirectory);

			ImGui::TreePop();
		}
	}

	static bool AssetButton(ImTextureID thumbnail, const Vector2u& aSize, const char* aName)
	{
		ImGui::PushID(aName);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
		ImGui::ImageButton("##assetButton", thumbnail, ImVec2(aSize.x, aSize.y));
		ImGui::PopStyleColor(2);

		bool wasDoubleClicked = ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("##ItemContextMenu");
		}

		if (ImGui::BeginPopup("##ItemContextMenu"))
		{
			if (ImGui::MenuItem("Test Button"))
			{

			}
			ImGui::EndPopup();
		}

		ImGui::TextWrapped("%s", aName);
		ImGui::PopID();
		return wasDoubleClicked;
	}

	void ContentBrowserPanel::DrawContentArea()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
		ImGui::BeginChild("##contentBrowserContentArea", ImVec2(0,0), ImGuiChildFlags_Borders);

		Icons* icons = Editor::Manager::Get()->GetIcons();

		static float padding = 16.0f;
		static float thumbnailSize = 128.0f;
		float cellSize = thumbnailSize + padding;
		float panelWidth = ImGui::GetContentRegionAvail().x;
		int32_t columnCount = static_cast<int32_t>(std::max(panelWidth / cellSize, 1.0f));

		const bool isAtBase = m_CurrentEntry->m_Parent == nullptr;

		// main content area
		if (ImGui::BeginTable("#contentTable", columnCount, ImGuiTableFlags_None))
		{
			if (!isAtBase)
			{
				ImGui::TableNextColumn();
				if (AssetButton((ImTextureID)icons->GetIcon(EDITOR_ICON_FOLDER).m_Texture.get(), Vector2u(thumbnailSize), "..."))
				{
					m_CurrentEntry = m_CurrentEntry->m_Parent;
				}
			}

			for (uint32_t i = 0; i < m_CurrentEntry->m_ChildDirectories.size(); ++i)
			{
				ImGui::TableNextColumn();
				if (AssetButton((ImTextureID)icons->GetIcon(EDITOR_ICON_FOLDER).m_Texture.get(), Vector2u(thumbnailSize), m_CurrentEntry->m_ChildDirectories[i].m_Path.filename().string().c_str()))
				{
					m_CurrentEntry = &m_CurrentEntry->m_ChildDirectories[i];
				}
			}

			for (uint32_t i = 0; i < m_CurrentEntry->m_Files.size(); ++i)
			{
				ImGui::TableNextColumn();
				// TODO: use thumbnail
				if (AssetButton((ImTextureID)icons->GetIcon(EDITOR_ICON_FILE).m_Texture.get(), Vector2u(thumbnailSize), m_CurrentEntry->m_Files[i].filename().string().c_str()))
				{
					// TODO: Open asset
				}
			}

			ImGui::EndTable();
		}

		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::GetEntriesForPath(ContentDirectory contentDirectory, const std::filesystem::path& currentEntryPath, DirectoryEntry& entry)
	{
		entry.m_Path = currentEntryPath;
		entry.m_ChildDirectories.clear();
		entry.m_Files.clear();
		entry.m_ContentDirectory = contentDirectory;

		std::filesystem::directory_iterator directoryIterator(currentEntryPath);
		for (const std::filesystem::directory_entry& directoryEntry : directoryIterator)
		{
			const std::filesystem::path p = currentEntryPath / directoryEntry.path().filename();
			if (directoryEntry.is_directory())
			{
				DirectoryEntry child;
				child.m_Parent = &entry;
				GetEntriesForPath(contentDirectory, p, child);
				entry.m_ChildDirectories.push_back(child);
			}
			else
				entry.m_Files.push_back(directoryEntry.path());
		}
	}

	void ContentBrowserPanel::UpdateParentsAndCurrentEntry(const std::filesystem::path& currentEntryPath, DirectoryEntry& entry)
	{
		if (currentEntryPath == entry.m_Path)
			m_CurrentEntry = &entry;

		for (uint32_t i = 0; i < entry.m_ChildDirectories.size(); ++i)
		{
			DirectoryEntry& childDir = entry.m_ChildDirectories[i];
			childDir.m_Parent = &entry;
			UpdateParentsAndCurrentEntry(currentEntryPath, childDir);
		}
	}
}
#endif //ENABLE_EDITOR