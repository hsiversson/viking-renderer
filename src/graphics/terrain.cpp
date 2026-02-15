#include "terrain.h"

#include "material.h"
#include "view.h"

namespace vkr::Graphics
{
	void TerrainRenderer::GenerateClipmapMesh(View* view)
	{
		const ViewRenderData& renderData = view->GetRenderData();
		Mesh* terrain = renderData.m_TerrainMesh;

		//Issue a pass to regenrate the verts and indices of the terrain clipmap on the currently used mesh
	}

	TerrainSceneObject::TerrainSceneObject(Ref<Terrain> terrain):
		m_Terrain(terrain)
	{
	}

	void TerrainSceneObject::CollectRenderObjects(ViewRenderData& renderData, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary)
	{
		Graphics::RenderObject obj;
		InstanceData data;
		data.m_Transform = Mat44::Identity();
		data.m_PrevTransform = Mat44::Identity();
		data.m_MaterialID = m_Terrain->m_TerrainMaterial->GatherMaterialData(renderData.m_MaterialDataBuffer); //TODO
		//Fill up instance data with RT specific info
		data.m_VertexBufferDescriptorIndex = m_Terrain->m_VertexBufferViews[m_CurrentMesh]->GetIndex();
		data.m_VertexStride = m_Terrain->m_VertexLayout.GetStride();
		data.m_VertexPositionByteOffset = m_Terrain->m_VertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_POSITION, 0);
		data.m_VertexNormalByteOffset = m_Terrain->m_VertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_NORMAL, 0);
		data.m_VertexTangentByteOffset = m_Terrain->m_VertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_TANGENT, 0);
		data.m_VertexUVByteOffset = m_Terrain->m_VertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_UV, 0);
		data.m_IndexBufferDescriptorIndex = m_Terrain->m_IndexBufferViews[m_CurrentMesh]->GetIndex();
		data.m_IndexStride = GetFormatBytesPerPixel(m_Terrain->m_IndexBuffers[m_CurrentMesh]->GetDesc().m_Format);
		uint8_t* genericdata = (uint8_t*)&data;
		//Serialize instance data into byte buffer
		obj.m_InstanceDataIndex = renderData.m_InstanceData.size();
		renderData.m_InstanceData.insert(renderData.m_InstanceData.end(), genericdata, genericdata + sizeof(InstanceData));
		//======================================
		renderData.m_TotalInstanceCount++;
		obj.m_VB = m_Terrain->m_VertexBuffers[m_CurrentMesh];
		obj.m_IB = m_Terrain->m_IndexBuffers[m_CurrentMesh];
		obj.m_Topology = Render::PrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		obj.m_VertexLayout = m_Terrain->m_VertexLayout;
		obj.m_Material = m_Terrain->m_TerrainMaterial.get();
		renderData.m_VisibleMeshes.push_back(obj);

		Render::RaytracingInstanceDesc rtInstanceDesc = {};
		rtInstanceDesc.m_BLAS = m_Terrain->m_BLAS[m_CurrentMesh];
		rtInstanceDesc.m_InstanceId = obj.m_InstanceDataIndex;
		rtInstanceDesc.m_Transform = data.m_Transform;
		rtInstanceDesc.m_HitGroupIndex = hitGroupLibrary.at(m_Terrain->m_TerrainMaterial->GetMaterial());
		renderData.m_RaytracingInstances.push_back(rtInstanceDesc);

		m_CurrentMesh++;
	}

	void TerrainSceneObject::GatherMaterials(std::unordered_set<Material*>& outMaterials)
	{
		outMaterials.insert(m_Terrain->m_TerrainMaterial->GetMaterial());
	}
}