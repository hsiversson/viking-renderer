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

		uint32_t partCounter = 0;
		for (const auto& part : m_Model->GetParts())
		{
			CollectModelPart(partCounter, renderData, part, m_World, m_PrevWorld);
		}

		m_PrevWorld = m_World;
	}

	void ModelObject::CollectRaytracingHitGroups(std::vector<Render::RaytracingHitGroupDesc>& outHitGroups)
	{
		if (!m_Model)
			return;

		m_MaterialHitGroupIndexOffset = outHitGroups.size();
		for (const auto& part : m_Model->GetParts())
		{
			CollectRaytracingHitGroup(outHitGroups, part);
		}
	}

	void ModelObject::CollectModelPart(uint32_t& partCounter, ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform)
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

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = obj.m_InstanceDataIndex;
			rtInstanceDesc.m_Transform = data.m_Transform;
			rtInstanceDesc.m_HitGroupIndex = m_MaterialHitGroupIndexOffset + partCounter;
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		++partCounter;
		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(partCounter, renderData, part.m_ChildParts[i], data.m_Transform, data.m_PrevTransform);
		}
	}

	void ModelObject::CollectRaytracingHitGroup(std::vector<Render::RaytracingHitGroupDesc>& outHitGroups, const Model::Part& part)
	{
		Material* material = part.m_Material->GetMaterial();
		Render::RaytracingHitGroupDesc hitGroupDesc = {};
		//material->GetHitGroupDesc(hitGroupDesc);
		outHitGroups.push_back(hitGroupDesc);

		for (const auto& part : part.m_ChildParts)
		{
			CollectRaytracingHitGroup(outHitGroups, part);
		}
	}

}