#include "scene.h"
#include "material.h"
#include "mesh.h"
#include "modelobject.h"
#include "render/device.h"
#include "sky.h"
#include "view.h"
#include "viewmanager.h"
#include "viewrenderer.h"

#include <functional>


namespace vkr::Graphics
{
	Scene::Scene()
	{
		m_TraceRaysDynamicShaderLib = Render::GetDevice()->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/tracerays_dynamic.hlsl"), nullptr, Render::SHADER_STAGE_RAYTRACING);
		m_ViewManager = MakeUnique<ViewManager>(*this);
		m_ViewRenderer = MakeUnique<ViewRenderer>();
		if (!m_ViewRenderer->Init())
		{
			VKR_ASSERT(false);
			return;
		}

		m_Sky = MakeUnique<Sky>();
		if (!m_Sky->Init())
		{
			VKR_ASSERT(false);
			return;
		}
	}

	Scene::~Scene()
	{

	}

	View* Scene::CreateView()
	{
		return m_ViewManager->CreateView();
	}

	void Scene::DestroyView(View* view)
	{
		m_ViewManager->DestroyView(view);
	}

	void Scene::Update()
	{
		std::queue<PendingAction> pendingActions;
		{
			std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
			pendingActions.swap(m_PendingActions);
		}

		// consume all pending actions
		
		std::unordered_set<Graphics::Material*> addedMaterials;
		PendingAction action = {};
		while (!pendingActions.empty())
		{
			action = pendingActions.front();
			pendingActions.pop();

			switch (action.m_Type)
			{
			case PendingAction::Type::Add:
			{
				switch (action.m_ObjectType)
				{
				case PendingAction::ObjectType::Model:
				{
					Ref<ModelSceneObject> model = MakeRef<ModelSceneObject>();
					model->SetModel(action.m_Model);
					m_Models.push_back(model);
					model->GatherMaterials(addedMaterials);
				}
				break;
				case PendingAction::ObjectType::LocalLight:
				{
					m_LocalLights.push_back(action.m_LocalLight);
				}
				break;
				case PendingAction::ObjectType::DirectionalLight:
				{
					m_DirectionalLights.push_back(action.m_DirectionalLight);
				}
				break;
				case PendingAction::ObjectType::Terrain:
				{
					VKR_ASSERT(!m_Terrain, "Only one terrain allowed!!");
					Ref<TerrainSceneObject> terrain = MakeRef<TerrainSceneObject>(action.m_Terrain);
					terrain->GatherMaterials(addedMaterials);
				}
				break;
				}
			}
			break;
			case PendingAction::Type::Remove:
			{
				switch (action.m_ObjectType)
				{
				case PendingAction::ObjectType::Model:
				{
					auto it = std::find_if(m_Models.begin(), m_Models.end(), [&action](Ref<ModelSceneObject> model) {return model->GetModel() == action.m_Model; });
					if (it != m_Models.end())
					{
						std::swap(*it, m_Models.back());
						m_Models.pop_back();
					}
				}
				break;
				case PendingAction::ObjectType::LocalLight:
				{
					auto it = std::find(m_LocalLights.begin(), m_LocalLights.end(), action.m_LocalLight);
					if (it != m_LocalLights.end())
					{
						std::swap(*it, m_LocalLights.back());
						m_LocalLights.pop_back();
					}
				}
				break;
				case PendingAction::ObjectType::DirectionalLight:
				{
					auto it = std::find(m_DirectionalLights.begin(), m_DirectionalLights.end(), action.m_DirectionalLight);
					if (it != m_DirectionalLights.end())
					{
						std::swap(*it, m_DirectionalLights.back());
						m_DirectionalLights.pop_back();
					}
				}
				break;
				case PendingAction::ObjectType::Terrain:
				{
					VKR_ASSERT(m_Terrain);
					m_Terrain.reset();
				}
				break;
				}
			}
			break;
			}
		}

		//Registers possible new hitgroups
		bool hitGroupAdded = false;
		for (auto material : addedMaterials)
		{
			if (!m_MaterialToHitGroupId.contains(material))
			{
				m_MaterialToHitGroupId[material] = m_MaterialHitGroupCounter++;
				m_HitGroupDescs.push_back(material->GetHitGroupDesc());
				hitGroupAdded = true;
			}
		}

		if (hitGroupAdded)
		{
			// update tracing pipeline state
			Render::PipelineStateDesc psoDesc = Render::PipelineStateDesc(Render::PIPELINE_STATE_TYPE_RAYTRACING);
			psoDesc.Raytracing.m_HitGroups = m_HitGroupDescs.data();
			psoDesc.Raytracing.m_NumHitGroups = m_HitGroupDescs.size();
			psoDesc.Raytracing.m_MissIdentifier = "Miss";
			psoDesc.Raytracing.m_RayGenerationIdentifier = "TraceRays";
			psoDesc.Raytracing.m_Shader = m_TraceRaysDynamicShaderLib.get();
			m_TraceRaysPipelineState = Render::GetDevice()->CreatePipelineState(psoDesc);
		}

		for (View* view : m_ViewManager->GetViews())
		{
			PrepareView(view);
			m_ViewRenderer->RenderView(view);
		}
	}

	void Scene::AddModel(const Ref<Model>& model)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Add;
		action.m_ObjectType = PendingAction::ObjectType::Model;
		action.m_Model = model;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::RemoveModel(const Ref<Model>& model)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Remove;
		action.m_ObjectType = PendingAction::ObjectType::Model;
		action.m_Model = model;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::AddLight(const Ref<LocalLight>& light)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Add;
		action.m_ObjectType = PendingAction::ObjectType::LocalLight;
		action.m_LocalLight = light;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::RemoveLight(const Ref<LocalLight>& light)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Remove;
		action.m_ObjectType = PendingAction::ObjectType::LocalLight;
		action.m_LocalLight = light;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::AddDirectionalLight(const Ref<DirectionalLight>& light)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Add;
		action.m_ObjectType = PendingAction::ObjectType::DirectionalLight;
		action.m_DirectionalLight = light;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::RemoveDirectionalLight(const Ref<DirectionalLight>& light)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Remove;
		action.m_ObjectType = PendingAction::ObjectType::DirectionalLight;
		action.m_DirectionalLight = light;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::AddTerrain(const Ref<Terrain>& terrain)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Add;
		action.m_ObjectType = PendingAction::ObjectType::Terrain;
		action.m_Terrain = terrain;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::RemoveTerrain(const Ref<Terrain>& terrain)
	{
		PendingAction action = {};
		action.m_Type = PendingAction::Type::Remove;
		action.m_ObjectType = PendingAction::ObjectType::Terrain;
		action.m_Terrain = terrain;

		std::unique_lock<std::mutex> lock(m_PendingActionsMutex);
		m_PendingActions.push(std::move(action));
	}

	void Scene::PrepareView(View* view)
	{
		// traverse all objects in Scene, add relevant ones to view.PrepareData()
		PrepareViewContext prepareViewCtx(view);
		ViewRenderData& prepareData = view->GetPrepareData();

		prepareData.m_FrameIndex = ElapsedTimer::FrameIndex();
		prepareData.m_DeltaTime = ElapsedTimer::DeltaTime();
		prepareData.m_ElapsedTime = ElapsedTimer::ElapsedTime();

		prepareData.m_RenderSize = view->GetRenderSize();
		prepareData.m_OutputSize = view->GetOutputSize();

		view->PrepareCameraConstants(prepareData.m_CameraData);

		prepareData.m_TraceRaysPipelineState = m_TraceRaysPipelineState;

		for (size_t i = 0; i < m_Models.size(); i++)
		{
			m_Models[i]->CollectRenderObjects(prepareData, m_MaterialToHitGroupId);
		}

		if (m_Terrain)
		{
			m_Terrain->CollectRenderObjects(prepareData, m_MaterialToHitGroupId);
		}

		std::sort(prepareData.m_VisibleMeshes.begin(), prepareData.m_VisibleMeshes.end());

		auto DepthPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDepthPipelineState(obj->m_VertexLayout);
		};
		auto DefaultPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDefaultPipelineState(obj->m_VertexLayout);
		};

		//Pass batch collection
		auto CollectBatchesForPass = [&prepareData](MeshPassData& PassData,std::function<Ref<Render::PipelineState>(RenderObject*)> PSOSelector) {
			if (!prepareData.m_VisibleMeshes.size())
				return;
			RenderObject* referenceObject = &prepareData.m_VisibleMeshes[0];
			RenderBatch currentBatch;
			currentBatch.m_VB = referenceObject->m_VB;
			currentBatch.m_IB = referenceObject->m_IB;
			currentBatch.m_Topology = referenceObject->m_Topology;
			currentBatch.m_PSO = PSOSelector(referenceObject);
			currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();

			for (auto it = prepareData.m_VisibleMeshes.begin(); it != prepareData.m_VisibleMeshes.end(); it++)
			{
				//For now material IDs well leave them (were not filling material parameters or using them anyway YET)
				if (referenceObject->m_VB == it->m_VB && referenceObject->m_IB == it->m_IB && PSOSelector(referenceObject) == PSOSelector(&(*it)))
				{
					currentBatch.m_Count++;
				}
				else
				{
					PassData.m_InstanceBatches.push_back(currentBatch);
					currentBatch.m_VB = it->m_VB;
					currentBatch.m_IB = it->m_IB;
					currentBatch.m_Topology = it->m_Topology;
					currentBatch.m_PSO = PSOSelector(&(*it));

					currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();
					currentBatch.m_Count = 1;
					referenceObject = &(*it);
				}
				prepareData.m_InstanceDataOffsetBuffer.push_back(it->m_InstanceDataIndex);
			}
			//Add last batch
			PassData.m_InstanceBatches.push_back(currentBatch);
		};
		
		CollectBatchesForPass(prepareData.m_DepthPassData, DepthPSOSelector);
		if (false) //(!useRaytracing)
		{
			CollectBatchesForPass(prepareData.m_ForwardPassData, DefaultPSOSelector);
		}

		// Process lights
		for (uint32_t i = 0; i < m_LocalLights.size(); ++i)
		{
			// frustum cull

			const Ref<LocalLight> light = m_LocalLights[i];
			prepareData.m_VisibleLights.push_back(*light);
		}

		for (uint32_t i = 0; (i < m_DirectionalLights.size()) && (i < 2); ++i)
		{
			prepareData.m_DirectionalLights[i] = *m_DirectionalLights[i];
			++prepareData.m_NumDirectionalLights;
		}

		m_Sky->PrepareView(view);
	}
	
}