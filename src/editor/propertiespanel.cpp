#include "propertiespanel.h"
#include "game/idcomponent.h"
#include "game/transformcomponent.h"
#include "game/lightcomponent.h"

#if ENABLE_EDITOR
namespace vkr::Editor
{
	PropertiesPanel::PropertiesPanel()
		: Panel("Properties")
	{

	}

	PropertiesPanel::~PropertiesPanel()
	{

	}

	static void DrawProperty(const char* name, float& property)
	{
		ImGui::PushID(name);
		ImGui::PushID(&property);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 128.0f);
		ImGui::Text("%s", name);
		ImGui::NextColumn();

		ImGui::DragFloat("##X", &property, 0.1f, 0.0f, 0.0f, "%.2f");
		//if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		//	aValue = aResetValue;

		ImGui::Columns(1);
		ImGui::PopID();
		ImGui::PopID();
	}

	static void DrawProperty(const char* name, Vector2f& property)
	{
		ImGui::PushID(name);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 128.0f);
		ImGui::Text("%s", name);
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.25f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.25f, 0.1f, 1.0f });
		ImGui::Button("X", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##X", &property.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::Button("Y", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Y", &property.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	static void DrawProperty(const char* name, Vector3f& property)
	{
		ImGui::PushID(name);

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, 128.0f);
		ImGui::Text("%s", name);
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.25f, 0.1f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.25f, 0.1f, 1.0f });
		ImGui::Button("X", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##X", &property.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::Button("Y", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Y", &property.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.15f, 0.1f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.2f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.15f, 0.1f, 0.8f, 1.0f });
		ImGui::Button("Z", buttonSize);
		ImGui::PopStyleColor(3);
		ImGui::SameLine();
		ImGui::DragFloat("##Z", &property.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	static void DrawProperty(const char* name, Rotator& property)
	{
		Vector3f rot = Vector3f(property.m_Pitch, property.m_Yaw, property.m_Roll);
		DrawProperty(name, rot);
		property = Rotator(rot);
	}

	void PropertiesPanel::OnDraw()
	{
		if (!m_SelectedEntities.empty())
		{
			Game::IdComponent& idComponent = m_SelectedEntities[0].GetComponent<Game::IdComponent>();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::InputText("##entityName", idComponent.m_Name.data(), idComponent.m_Name.length(), 0);
			ImGui::PopItemFlag();

			if (m_SelectedEntities[0].HasComponent<Game::TransformComponent>())
			{
				ImGui::Separator();
				Game::TransformComponent& transform = m_SelectedEntities[0].GetComponent<Game::TransformComponent>();
				ForEachProperty<Game::TransformComponent>([&](auto&& property)
					{
						DrawProperty(property.m_Name, transform.*(property.m_Member));
					});
			}
			if (m_SelectedEntities[0].HasComponent<Game::DirectionalLightComponent>())
			{
				ImGui::Separator();
				Game::DirectionalLightComponent& dirLight = m_SelectedEntities[0].GetComponent<Game::DirectionalLightComponent>();
				ForEachProperty<Game::DirectionalLightComponent>([&](auto&& property)
					{
						DrawProperty(property.m_Name, dirLight.*(property.m_Member));
					});
			}
		}
	}
	void PropertiesPanel::ReceiveMessage(const BroadcastMessage& message)
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
#endif