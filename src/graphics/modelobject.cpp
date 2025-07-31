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
			CollectModelPart(renderData, part, m_World, m_PrevWorld);
		}

		m_PrevWorld = m_World;
	}

	void ModelObject::CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform)
	{
		Graphics::RenderObject obj;
		InstanceData data;
		data.m_Transform = part.m_LocalTransform * parentWorldTransform;
		data.m_PrevTransform = part.m_LocalTransform * prevParentWorldTransform;
		data.m_MaterialID = part.m_Material->GatherMaterialData(renderData.m_MaterialDataBuffer); //TODO
		//Fill up instance data with RT specific info
		data.m_VertexBufferDescriptorIndex = part.m_Mesh->GetRaytraceVBView()->GetIndex();
		data.m_VertexStride = part.m_Mesh->GetVertexLayout().GetStride();
		data.m_VertexPositionByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_POSITION, 0);
		data.m_VertexNormalByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_NORMAL, 0);
		data.m_VertexTangentByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_TANGENT, 0);
		data.m_VertexUVByteOffset = part.m_Mesh->GetVertexLayout().GetByteOffset(Render::VertexAttribute::TYPE_UV, 0);
		data.m_IndexBufferDescriptorIndex = part.m_Mesh->GetRaytraceIBView()->GetIndex();
		data.m_IndexStride = GetFormatBytesPerPixel(part.m_Mesh->GetIndexBuffer()->GetDesc().m_Format);
		uint8_t* genericdata = (uint8_t*) &data;
		//Serialize instance data into byte buffer
		obj.m_InstanceDataIndex = renderData.m_InstanceData.size();
		renderData.m_InstanceData.insert(renderData.m_InstanceData.end(),genericdata, genericdata + sizeof(InstanceData));
		//======================================
		renderData.m_TotalInstanceCount++;
		obj.m_Mesh = part.m_Mesh.get();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		Render::RaytracingHitGroupDesc hitGroupDesc = {};
		part.m_Material->GetMaterial()->GetHitGroupDesc(hitGroupDesc);
		renderData.m_MaterialHitGroups.push_back(hitGroupDesc);

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = obj.m_InstanceDataIndex;
			rtInstanceDesc.m_Transform = data.m_Transform;
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], data.m_Transform, data.m_PrevTransform);
		}
	}

}