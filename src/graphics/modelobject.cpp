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
		obj.m_Transform = parentWorldTransform * part.m_LocalTransform;
		obj.m_Mesh = part.m_Mesh.get();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], obj.m_Transform);
		}
	}

}