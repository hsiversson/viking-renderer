#include "modelobject.h"

#include "model.h"

namespace vkr::Graphics
{

	ModelObject::ModelObject()
	{

	}

	ModelObject::~ModelObject()
	{

	}

	void ModelObject::CollectRenderObjects(ViewRenderData& renderData)
	{
		if (!m_Model)
			return;

		for (const auto& part : m_Model->GetParts())
		{
			CollectModelPart(renderData, part, GetWorldTransform());
		}
	}

	void ModelObject::CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform)
	{
		Graphics::RenderObject obj;
		InstanceData data;
		data.m_Transform = part.m_LocalTransform * parentWorldTransform;
		data.m_MaterialID = part.m_Material->GatherMaterialData(renderData.m_MaterialDataBuffer); //TODO
		uint8_t* genericdata = (uint8_t*) &data;
		//Serialize instance data into byte buffer
		obj.m_InstanceDataIndex = renderData.m_InstanceData.size();
		renderData.m_InstanceData.insert(renderData.m_InstanceData.end(),genericdata, genericdata + sizeof(InstanceData));
		//======================================
		renderData.m_TotalInstanceCount++;
		obj.m_Mesh = part.m_Mesh.get();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = 0;
			rtInstanceDesc.m_Transform = data.m_Transform;
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], data.m_Transform);
		}
	}

}