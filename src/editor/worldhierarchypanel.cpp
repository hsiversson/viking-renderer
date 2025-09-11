#include "worldhierarchypanel.h"

#if ENABLE_EDITOR
#include "editor.h"

#include "game/idcomponent.h"
#include "game/world.h"

namespace vkr::Editor
{

	WorldHierarchyPanel::WorldHierarchyPanel(Game::World& world)
		: Panel("World Hierarchy")
		, m_World(world)
	{

	}

	WorldHierarchyPanel::~WorldHierarchyPanel()
	{

	}

	void WorldHierarchyPanel::OnUpdate()
	{

	}

	void WorldHierarchyPanel::OnDraw()
	{
		if (ImGui::BeginTable("#propertyTable", 2, ImGuiTableFlags_Resizable))
		{
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_IndentEnable, 96.0f);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_IndentDisable);
			ImGui::TableHeadersRow();

			Game::EntityRegistry& entityRegistry = m_World.GetEntityRegistry();

			const std::vector<Game::IdComponent>* allEntities = entityRegistry.ViewComponents<Game::IdComponent>();
			if (allEntities)
			{
				for (uint32_t i = 0; i < allEntities->size(); i++)
				{
					Game::Entity entity = Game::Entity((*allEntities)[i].m_Uid, &entityRegistry);
					if (entity.GetParent()) // skip non roots
						continue;

					DrawEntityNode(entity);
				}
			}

			ImGui::EndTable();
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			m_SelectedEntity = Game::Entity();

			BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITY);
			msg.SetData(m_SelectedEntity);
			Manager::Get()->Broadcast(msg);
		}
	}

	void WorldHierarchyPanel::DrawEntityNode(const Game::Entity& entity)
	{
		ImGui::TableNextRow();

		const Game::IdComponent* idComponent = entity.GetComponent<Game::IdComponent>();
		///Gfw_VisibleComponent& visibleComponent = aEntity.GetComponent<Gfw_VisibleComponent>();
		ImGui::PushID(idComponent->m_Name.c_str());

		// Visible toggle
		ImGui::TableSetColumnIndex(0);

		Icons* icons = Manager::Get()->GetIcons();
		Render::TextureView* visibleIcon = icons->GetIcon(EDITOR_ICON_VISIBLE_WHITE).m_Texture.get();
		Render::TextureView* notVisibleIcon = icons->GetIcon(EDITOR_ICON_NOT_VISIBLE_WHITE).m_Texture.get();

		if (ImGui::ImageButton("##entityVisible", (ImTextureID)visibleIcon, {20.f, 20.f}))
		{
		//	visibleComponent.mIsVisible = !visibleComponent.mIsVisible;
		//	// OnVisibleChanged?
		}

		ImGui::SameLine();

		ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;

		bool isSelected = entity == m_SelectedEntity;
		if (isSelected)
			treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

		const std::vector<Game::Entity>& children = entity.GetChildren();
		const bool hasChildren = !children.empty();
		if (!hasChildren)
			treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx(idComponent->m_Name.c_str(), treeNodeFlags);

		if (ImGui::BeginDragDropSource())
		{
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked())
		{
			m_SelectedEntity = entity;
			
			BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITY);
			msg.SetData(m_SelectedEntity);
			Manager::Get()->Broadcast(msg);
		}

		// Right click menu
		//bool entityDeleted = false;
		//if (ImGui::BeginPopupContextItem())
		//{
		//	mSelectedEntity = aEntity;
		//	Editor_BaseModule::Get()->GetRoot()->SetSelectedEntity(mSelectedEntity);
		//	if (ImGui::MenuItem("Delete"))
		//		entityDeleted = true;
		//
		//	ImGui::EndPopup();
		//}

		// Type
		ImGui::TableSetColumnIndex(1);
		ImGui::TextDisabled(idComponent->m_Type.c_str());

		if (opened)
		{
			if (hasChildren)
			{
				for (const Game::Entity& child : children)
				{
					DrawEntityNode(child);
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

}

#endif //ENABLE_EDITOR