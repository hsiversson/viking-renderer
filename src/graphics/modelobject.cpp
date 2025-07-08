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
		obj.m_Transform = part.m_LocalTransform * parentWorldTransform;
		obj.m_Mesh = part.m_Mesh.get();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = 0;
			rtInstanceDesc.m_Transform = obj.m_Transform;
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], obj.m_Transform);
		}
	}

}