#include "propertiespanel.h"
#include "game/idcomponent.h"

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

	void PropertiesPanel::OnDraw()
	{
		if (m_SelectedEntity.IsValid())
		{
			Game::IdComponent* idComponent = m_SelectedEntity.GetComponent<Game::IdComponent>();
			ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
			ImGui::InputText("##entityName", idComponent->m_Name.data(), idComponent->m_Name.length(), 0);
			ImGui::PopItemFlag();
			ImGui::Separator();
		}
	}
	void PropertiesPanel::ReceiveMessage(const BroadcastMessage& message)
	{
		if (message.GetId() == BROADCAST_MSG_ID_SELECTED_ENTITY)
		{
			message.GetData(m_SelectedEntity);
		}
	}
}
#endif