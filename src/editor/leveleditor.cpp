#include "leveleditor.h"

#include "contentbrowserpanel.h"
#include "game/hierarchycomponent.h"
#include "game/modelcomponent.h"
#include "game/transformcomponent.h"
#include "game/lightcomponent.h"
#include "game/world.h"
#include "graphics/scene.h"
#include "propertiespanel.h"
#include "viewportpanel.h"
#include "worldhierarchypanel.h"

// TEMP
#include "graphics/modelloader_gltf.h"
// TEMP

#if ENABLE_EDITOR

namespace vkr::Editor
{
	LevelEditor::LevelEditor()
		: m_Mode(Mode::Editing)
	{
		m_World = MakeUnique<Game::World>();
		m_Viewport = MakeRef<ViewportPanel>(*m_World);
		m_ContentBrowser = MakeRef<ContentBrowserPanel>();
		m_WorldHierarchy = MakeRef<WorldHierarchyPanel>(*m_World);
		m_Properties = MakeRef<PropertiesPanel>();
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_Viewport));
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_ContentBrowser));
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_WorldHierarchy));
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_Properties));

		{
			Game::Entity e0 = m_World->CreateEntity("CP_Noodles");
			Game::ModelComponent& modelComponent = e0.AddComponent<Game::ModelComponent>();
			modelComponent.m_ModelFilePath = SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "models/cp_noodles/scene.gltf");

			// load something for now
			Graphics::ModelLoader_GLTF loader;
			modelComponent.m_Model = loader.Load(modelComponent.m_ModelFilePath);
			m_World->GetGraphicsScene()->AddModel(modelComponent.m_Model);
		}

		{
			Game::Entity sunEntity = m_World->CreateEntity("Sun");
			Game::DirectionalLightComponent& dirLight = sunEntity.AddComponent<Game::DirectionalLightComponent>();
			Game::TransformComponent& transform = sunEntity.AddComponent<Game::TransformComponent>();
			transform.m_Rotation = Rotator(-50.0f, -10.0f, 0.0f);
			
			m_World->GetGraphicsScene()->AddDirectionalLight(dirLight.m_Light);
		}
	}

	LevelEditor::~LevelEditor()
	{
		m_Panels.clear();
	}

	void LevelEditor::OnUpdate()
	{
		switch (m_Mode)
		{
		case Mode::Editing:
		{
			{
				// for now update all model transforms here...
				auto modelComponents = m_World->GetEntityRegistry().view<Game::ModelComponent>();
				for (const Game::EntityHandle entityHandle : modelComponents)
				{
					Game::Entity entity = Game::Entity(entityHandle, &m_World->GetEntityRegistry());
					Game::TransformComponent& transform = entity.GetComponent<Game::TransformComponent>();
					Game::ModelComponent& model = entity.GetComponent<Game::ModelComponent>();
					model.m_Model->SetTransform(Compose(transform.m_Position, transform.m_Rotation.ToQuaternion(), transform.m_Scale));
				}
			}
			{
				auto dirLightComponents = m_World->GetEntityRegistry().view<Game::DirectionalLightComponent>();
				for (const Game::EntityHandle entityHandle : dirLightComponents)
				{
					Game::Entity entity = Game::Entity(entityHandle, &m_World->GetEntityRegistry());
					Game::TransformComponent& transformComponent = entity.GetComponent<Game::TransformComponent>();
					Game::DirectionalLightComponent& dirLight = entity.GetComponent<Game::DirectionalLightComponent>();
					Mat44 transform = Compose(transformComponent.m_Position, transformComponent.m_Rotation.ToQuaternion(), transformComponent.m_Scale);
					dirLight.m_Light->Direction = Normalized(Vector3f(transform.At(2, 0), transform.At(2, 1), transform.At(2, 2)));
					dirLight.m_Light->Emission = dirLight.m_Color * dirLight.m_Intensity;
					dirLight.m_Light->Radius = DegToRad(dirLight.m_Radius);
				}
			}

			m_World->Update();
		}
		break;
		case Mode::Playing:
		{

		}
		}
	}

	void LevelEditor::OnDraw()
	{

	}

	void LevelEditor::SetMode(Mode mode)
	{
		m_Mode = mode;
	}

}

#endif //ENABLE_EDITOR