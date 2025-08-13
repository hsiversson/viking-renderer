#include "leveleditor.h"

#include "contentbrowserpanel.h"
#include "graphics/model.h"
#include "graphics/modelloader_gltf.h"
#include "graphics/modelobject.h"
#include "graphics/scene.h"
#include "viewportpanel.h"

#if ENABLE_EDITOR

namespace vkr::Editor
{

	LevelEditor::LevelEditor()
		: m_Mode(Mode::Editing)
	{
		m_Scene = MakeUnique<Graphics::Scene>();
		m_Viewport = MakeRef<ViewportPanel>(m_Scene.get());
		m_ContentBrowser = MakeRef<ContentBrowserPanel>();
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_Viewport));
		m_Panels.push_back(std::static_pointer_cast<Panel>(m_ContentBrowser));

		// load something for now
		Graphics::ModelLoader_GLTF loader;
		Ref<Graphics::Model> model;
		model = loader.Load(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "models/cp_noodles/scene.gltf"));
		Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
		modelinst->SetLocalTransform(Compose(Mat33::Identity(), Vector3f(0.0f, 0.0f, 0.0f)));

		modelinst->SetModel(model);
		m_Scene->AddObject(modelinst);
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
			m_Scene->Update();
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