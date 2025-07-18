#include "viewrenderdata.h"

namespace vkr::Graphics
{
	void ViewRenderData::Clear()
	{
		m_RaytracingTLAS = {};
		m_RaytracingInstances.clear();

		m_VisibleMeshes.clear();
		m_VisibleLights.clear();
		m_DepthPassData = {};
		m_ForwardPassData = {};

		m_InstanceData.clear(); 
		m_InstanceDataBufferView = {};

		m_InstanceDataOffsetBuffer.clear();
		m_InstanceDataOffsetBufferView = {};

		m_MaterialDataBuffer.Clear();

		m_PerSceneConstantBuffer = {};

		m_TotalInstanceCount = 0;
	}
}
