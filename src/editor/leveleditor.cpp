#include "leveleditor.h"

#include "contentbrowserpanel.h"
#include "game/hierarchycomponent.h"
#include "game/modelcomponent.h"
#include "game/transformcomponent.h"
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

		Game::Entity e0 = m_World->CreateEntity("Test");
		Game::Entity e1 = m_World->CreateEntity("Test2");
		Game::Entity e2 = m_World->CreateEntity("Test3");
		Game::Entity e3 = m_World->CreateEntity("Test4");

		e0.AddChild(e1);
		e0.AddChild(e2);
		e0.AddChild(e3);

		Game::ModelComponent& modelComponent = e0.AddComponent<Game::ModelComponent>();
		modelComponent.m_ModelFilePath = SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "models/cp_noodles/scene.gltf");

		// load something for now
		Graphics::ModelLoader_GLTF loader;
		modelComponent.m_Model = loader.Load(modelComponent.m_ModelFilePath);
		m_World->GetGraphicsScene()->AddModel(modelComponent.m_Model);
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
			// for now update all model transforms here...
			std::vector<Game::EntityHandle>* modelComponents = m_World->GetEntityRegistry().ViewEntities<Game::ModelComponent>();
			for (const Game::EntityHandle entity : *modelComponents)
			{
				Game::Entity e = Game::Entity(entity, &m_World->GetEntityRegistry());
				Game::TransformComponent* transform = e.GetComponent<Game::TransformComponent>();
				Game::ModelComponent* model = e.GetComponent<Game::ModelComponent>();
				model->m_Model->SetTransform(Compose(transform->m_Position, transform->m_Rotation, transform->m_Scale));
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