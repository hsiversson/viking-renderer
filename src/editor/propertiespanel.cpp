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

				ImGui::DragFloat3("Position", &transform.m_Position.x, 0.1f);

				if (transform.m_Rotation != m_PrevRotation)
				{
					m_PrevRotation = transform.m_Rotation;
					m_EulerRotationCacheInitialized = false;
				}

				if (!m_EulerRotationCacheInitialized)
				{
					m_EulerRotationCache = transform.m_Rotation;
					m_EulerRotationCacheInitialized = true;
				}

				Rotator eulerUI = m_EulerRotationCache;
				if (ImGui::DragFloat3("Rotation", &transform.m_Rotation.m_Pitch, 0.5f))
				{
					//Vector3f delta;
					//for (uint32_t i = 0; i < 3; ++i)
					//{
					//	delta[i] = eulerUI[i] - m_EulerRotationCache[i];
					//	while (delta[i] > 180.f) delta[i] -= 360.f;
					//	while (delta[i] < -180.f) delta[i] += 360.f;
					//}
					//
					//Quaternion qDelta = Quaternion::FromEuler(delta);
					//transform.m_Rotation = transform.m_Rotation * qDelta;
					//transform.m_Rotation.Normalize();
					//
					//m_EulerRotationCache = eulerUI;
				}

				ImGui::DragFloat3("Scale", &transform.m_Scale.x, 0.1f);
			}
			if (m_SelectedEntities[0].HasComponent<Game::DirectionalLightComponent>())
			{
				ImGui::Separator();
				Game::DirectionalLightComponent& dirLight = m_SelectedEntities[0].GetComponent<Game::DirectionalLightComponent>();
				ImGui::ColorEdit3("Light Color", &dirLight.m_Color.x);
				ImGui::DragFloat("Intensity", &dirLight.m_Intensity, 0.1f, 0.0f, 120000.0f);
				ImGui::DragFloat("Radius", &dirLight.m_Radius, 0.01f, 0.0f, 90.0f);
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