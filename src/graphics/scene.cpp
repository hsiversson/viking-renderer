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
		bool hitGroupAdded = false;
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
					m_Models.push_back(action.m_Model);
					for (const Model::Part& part : action.m_Model->GetParts())
					{
						Material* material = part.m_Material->GetMaterial();
						if (!m_MaterialToHitGroupId.contains(material))
						{
							m_MaterialToHitGroupId[material] = m_MaterialHitGroupCounter++;
							m_HitGroupDescs.push_back(material->GetHitGroupDesc());
							hitGroupAdded = true;
						}
					}
				}
				break;
				case PendingAction::ObjectType::LocalLight:
				{

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
					auto it = std::find(m_Models.begin(), m_Models.end(), action.m_Model);
					if (it != m_Models.end())
					{
						std::swap(*it, m_Models.back());
						m_Models.pop_back();
					}
				}
				break;
				case PendingAction::ObjectType::LocalLight:
				{

				}
				break;
				}
			}
			break;
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

	const std::vector<Ref<Model>>& Scene::GetModels() const
	{
		return m_Models;
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

		//Sun
		prepareData.m_DirectionalLights[0].Emission = Vector3f(3.0, 3.0, 3.0);
		prepareData.m_DirectionalLights[0].Direction = Vector3f(0.4, -0.5, 0.6);
		prepareData.m_DirectionalLights[0].Radius = DegToRad(0.53f);

		//Moon
		prepareData.m_DirectionalLights[1].Emission = Vector3f(8.0, 2.0, 2.0);
		prepareData.m_DirectionalLights[1].Direction = Vector3f(-0.4, -0.5, 0.6);
		prepareData.m_DirectionalLights[1].Radius = 0.02f;

		view->PrepareCameraConstants(prepareData.m_CameraData);

		prepareData.m_TraceRaysPipelineState = m_TraceRaysPipelineState;

		for (size_t i = 0; i < m_Models.size(); i++)
		{
			Model* model = m_Models[i].get();
			for (const Model::Part& part : model->GetParts())
			{
				CollectModelPart(prepareData, part, model->GetTransform(), model->GetTransform());
			}
		}

		std::sort(prepareData.m_VisibleMeshes.begin(), prepareData.m_VisibleMeshes.end());

		auto DepthPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDepthPipelineState(obj->m_Mesh->GetVertexLayout());
		};
		auto DefaultPSOSelector = [](RenderObject* obj)->Ref<Render::PipelineState>
		{
			return obj->m_Material->GetMaterial()->GetDefaultPipelineState(obj->m_Mesh->GetVertexLayout());
		};

		//Pass batch collection
		auto CollectBatchesForPass = [&prepareData](MeshPassData& PassData,std::function<Ref<Render::PipelineState>(RenderObject*)> PSOSelector) {
			if (!prepareData.m_VisibleMeshes.size())
				return;
			RenderObject* referenceObject = &prepareData.m_VisibleMeshes[0];
			RenderBatch currentBatch;
			currentBatch.m_Mesh = referenceObject->m_Mesh;
			currentBatch.m_PSO = PSOSelector(referenceObject);
			currentBatch.m_StartOffset = prepareData.m_InstanceDataOffsetBuffer.size();

			for (auto it = prepareData.m_VisibleMeshes.begin(); it != prepareData.m_VisibleMeshes.end(); it++)
			{
				//For now material IDs well leave them (were not filling material parameters or using them anyway YET)
				if (referenceObject->m_Mesh == it->m_Mesh && PSOSelector(referenceObject) == PSOSelector(&(*it)))
				{
					currentBatch.m_Count++;
				}
				else
				{
					PassData.m_InstanceBatches.push_back(currentBatch);
					currentBatch.m_Mesh = it->m_Mesh;
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

		m_Sky->PrepareView(view);
	}

	void Scene::CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform)
	{
		Graphics::RenderObject obj;
		InstanceData data;
		data.m_Transform = part.m_LocalTransform * parentWorldTransform;
		data.m_PrevTransform = part.m_LocalTransform * prevParentWorldTransform;
		data.m_MaterialID = part.m_Material->GatherMaterialData(renderData.m_MaterialDataBuffer); //TODO
		//Fill up instance data with RT specific info
		data.m_VertexBufferDescriptorIndex = part.m_Mesh->GetVertexBufferView()->GetIndex();
		data.m_VertexStride = part.m_Mesh->GetVertexLayout().GetStride();
		data.m_VertexPositionByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_POSITION, 0);
		data.m_VertexNormalByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_NORMAL, 0);
		data.m_VertexTangentByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_TANGENT, 0);
		data.m_VertexUVByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_UV, 0);
		data.m_IndexBufferDescriptorIndex = part.m_Mesh->GetIndexBufferView()->GetIndex();
		data.m_IndexStride = GetFormatBytesPerPixel(part.m_Mesh->GetIndexBuffer()->GetDesc().m_Format);
		uint8_t* genericdata = (uint8_t*)&data;
		//Serialize instance data into byte buffer
		obj.m_InstanceDataIndex = renderData.m_InstanceData.size();
		renderData.m_InstanceData.insert(renderData.m_InstanceData.end(), genericdata, genericdata + sizeof(InstanceData));
		//======================================
		renderData.m_TotalInstanceCount++;
		obj.m_Mesh = part.m_Mesh.get();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = obj.m_InstanceDataIndex;
			rtInstanceDesc.m_Transform = data.m_Transform;
			rtInstanceDesc.m_HitGroupIndex = m_MaterialToHitGroupId.at(part.m_Material->GetMaterial());
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], data.m_Transform, data.m_PrevTransform);
		}
	}
}