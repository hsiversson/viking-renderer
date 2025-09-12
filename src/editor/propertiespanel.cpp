#include "propertiespanel.h"
#include "game/idcomponent.h"
#include "game/transformcomponent.h"

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

			if (m_SelectedEntity.HasComponent<Game::TransformComponent>())
			{
				Game::TransformComponent* transform = m_SelectedEntity.GetComponent<Game::TransformComponent>();

				ImGui::DragFloat3("Position", &transform->m_Position.x, 0.1f);

				if (transform->m_Rotation != m_PrevRotation)
				{
					m_PrevRotation = transform->m_Rotation;
					m_EulerRotationCacheInitialized = false;
				}

				if (!m_EulerRotationCacheInitialized)
				{
					m_EulerRotationCache = transform->m_Rotation.ToEuler();
					m_EulerRotationCacheInitialized = true;
				}

				Vector3f eulerUI = m_EulerRotationCache;
				if (ImGui::DragFloat3("Rotation", &eulerUI.x, 0.5f))
				{
					Vector3f delta = eulerUI - m_EulerRotationCache;

					for (int i = 0; i < 3; ++i)
					{
						while (delta[i] > 180.f) delta[i] -= 360.f;
						while (delta[i] < -180.f) delta[i] += 360.f;
					}

					Quaternion qDelta = Quaternion::FromEuler(delta);
					transform->m_Rotation = transform->m_Rotation * qDelta;
					transform->m_Rotation.Normalize();

					m_EulerRotationCache = eulerUI;
				}

				ImGui::DragFloat3("Scale", &transform->m_Scale.x, 0.1f);
			}
		}
	}
	void PropertiesPanel::ReceiveMessage(const BroadcastMessage& message)
	{
		if (message.GetId() == BROADCAST_MSG_ID_SELECTED_ENTITY)
		{
			message.GetData(m_SelectedEntity);
			m_EulerRotationCacheInitialized = false;
		}
	}
}
#endif