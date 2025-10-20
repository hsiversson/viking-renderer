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

			auto allEntities = entityRegistry.view<Game::IdComponent>();
			for (Game::EntityHandle e : allEntities)
			{
				Game::Entity entity = Game::Entity(allEntities.get<Game::IdComponent>(e).m_Uid, &entityRegistry);
				if (entity.GetParent()) // skip non roots
					continue;

				DrawEntityNode(entity);
			}

			ImGui::EndTable();
		}

		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
		{
			m_SelectedEntities.clear();

			BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITIES);
			struct Selection
			{
				uint32_t m_NumEntities;
				Game::Entity* m_Entities;
			};
			Selection selection = {};
			msg.SetData(selection);
			Manager::Get()->Broadcast(msg);
		}
	}

	void WorldHierarchyPanel::ReceiveMessage(const BroadcastMessage& message)
	{
		if (message.GetId() == BROADCAST_MSG_ID_SELECTED_ENTITIES)
		{
			struct Selection
			{
				uint32_t m_NumEntities;
				Game::Entity* m_Entities;
			};
			Selection selection;
			message.GetData(selection);

			m_SelectedEntities.clear();
			if (selection.m_NumEntities > 0)
			{
				m_SelectedEntities.insert(m_SelectedEntities.end(), selection.m_Entities, selection.m_Entities + selection.m_NumEntities);
			}
		}
	}

	void WorldHierarchyPanel::DrawEntityNode(const Game::Entity& entity)
	{
		ImGui::TableNextRow();

		const Game::IdComponent& idComponent = entity.GetComponent<Game::IdComponent>();
		///Gfw_VisibleComponent& visibleComponent = aEntity.GetComponent<Gfw_VisibleComponent>();
		ImGui::PushID(idComponent.m_Name.c_str());

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

		bool isSelected = std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end();
		if (isSelected)
			treeNodeFlags |= ImGuiTreeNodeFlags_Selected;

		const std::vector<Game::Entity>& children = entity.GetChildren();
		const bool hasChildren = !children.empty();
		if (!hasChildren)
			treeNodeFlags |= ImGuiTreeNodeFlags_Leaf;

		bool opened = ImGui::TreeNodeEx(idComponent.m_Name.c_str(), treeNodeFlags);

		if (ImGui::BeginDragDropSource())
		{
			ImGui::EndDragDropSource();
		}
		if (ImGui::BeginDragDropTarget())
		{
			ImGui::EndDragDropTarget();
		}

		if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
		{
			// single selection
			m_SelectedEntities.clear();
			m_SelectedEntities.push_back(entity);

			BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITIES);
			struct Selection
			{
				uint32_t m_NumEntities;
				Game::Entity* m_Entities;
			};
			Selection selection = {};
			selection.m_NumEntities = m_SelectedEntities.size();
			selection.m_Entities = m_SelectedEntities.data();
			msg.SetData(selection);
			Manager::Get()->Broadcast(msg);
		}
		else if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("##EntityContextMenu");
		}
		//else if (multiSelect)
		//{
		//	// TODO: Handle multi selection
		//}

		// Right click menu
		if (ImGui::BeginPopup("##EntityContextMenu"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				m_World.DestroyEntity(entity);

				BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITIES);
				struct Selection
				{
					uint32_t m_NumEntities;
					Game::Entity* m_Entities;
				};
				Selection selection = {};
				msg.SetData(selection);
				Manager::Get()->Broadcast(msg);
			}
			ImGui::EndPopup();
		}

		// Type
		ImGui::TableSetColumnIndex(1);
		ImGui::TextDisabled(idComponent.m_Type.c_str());

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