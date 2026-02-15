#include "modelobject.h"

#include "model.h"

namespace vkr::Graphics
{

	ModelSceneObject::ModelSceneObject()
	{

	}

	ModelSceneObject::~ModelSceneObject()
	{

	}

	void ModelSceneObject::CollectRenderObjects(ViewRenderData& renderData, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary)
	{
		if (!m_Model)
			return;

		uint32_t partCounter = 0;
		for (const Model::Part& part : m_Model->GetParts())
		{
			CollectModelPart(renderData, part, hitGroupLibrary, m_Model->GetTransform(), m_Model->GetTransform());
		}
	}


	void ModelSceneObject::CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform)
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
		obj.m_VB = part.m_Mesh->GetVertexBuffer();
		obj.m_IB = part.m_Mesh->GetIndexBuffer();
		obj.m_Topology = part.m_Mesh->GetTopology();
		obj.m_VertexLayout = part.m_Mesh->GetVertexLayout();
		obj.m_Material = part.m_Material.get();
		renderData.m_VisibleMeshes.push_back(obj);

		if (Ref<Render::Buffer> blas = part.m_Mesh->GetBLAS())
		{
			Render::RaytracingInstanceDesc rtInstanceDesc = {};
			rtInstanceDesc.m_BLAS = blas;
			rtInstanceDesc.m_InstanceId = obj.m_InstanceDataIndex;
			rtInstanceDesc.m_Transform = data.m_Transform;
			rtInstanceDesc.m_HitGroupIndex = hitGroupLibrary.at(part.m_Material->GetMaterial());
			renderData.m_RaytracingInstances.push_back(rtInstanceDesc);
		}

		for (uint32_t i = 0; i < part.m_ChildParts.size(); ++i)
		{
			CollectModelPart(renderData, part.m_ChildParts[i], hitGroupLibrary, data.m_Transform, data.m_PrevTransform);
		}
	}

	void ModelSceneObject::GatherMaterials(std::unordered_set<Graphics::Material*>& outMaterials)
	{
		for (const Model::Part& part : m_Model->GetParts())
		{
			Material* material = part.m_Material->GetMaterial();
			outMaterials.insert(material);
		}
	}

}