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

	void ModelObject::CollectRenderObjects(ViewRenderData& renderdata)
	{
		for (const auto& part : m_Model->GetParts())
		{
			Graphics::RenderObject obj;
			obj.m_Transform = GetWorldTransform();
			obj.m_Mesh = part.m_Mesh.get();
			obj.m_Material = part.m_Material.get();
			renderdata.m_VisibleMeshes.push_back(obj);
		}
		
	}

}