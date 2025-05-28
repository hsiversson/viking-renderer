#include "scene.h"
#include "view.h"

namespace vkr::Graphics
{
	Scene::Scene()
	{

	}

	Scene::~Scene()
	{

	}

	Ref<View> Scene::CreateView()
	{
		Ref<View> view = MakeRef<View>();
		m_Views.push_back(view);
		return view;
	}

	void Scene::DestroyView(const Ref<View>& view)
	{
		const auto offset = std::find(m_Views.begin(), m_Views.end(), view);
		m_Views[offset - m_Views.begin()] = m_Views.back();
		m_Views.pop_back();
	}

	void Scene::Update()
	{
		// run updates

		// sort views based on frame structure?
		// main view goes last usually
	}

	void Scene::PrepareView(View& view)
	{
		// traverse all objects in Scene, add relevant ones to view.PrepareData()

		ViewRenderData& prepareData = view.GetPrepareData();

		for (const auto& object : m_SceneObjects)
		{
			//RenderObject renderObj;
			//renderObj.m_Mesh = object->GetMesh();
			//renderObj.m_Material = object->GetMaterial();
			//renderObj.m_Transform = object->GetWorldTransform();
			//prepareData.m_VisibleMeshes.push_back(renderObj);
		}

		std::sort(prepareData.m_VisibleMeshes.begin(), prepareData.m_VisibleMeshes.end());
	}
}