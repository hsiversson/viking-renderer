#include "viewrenderdata.h"

namespace vkr::Graphics
{
	void ViewRenderData::Clear()
	{
		m_RaytracingTLAS.reset();
		m_RaytracingInstances.clear();

		m_VisibleMeshes.clear();
		m_VisibleLights.clear();
		m_DepthPassData = {};
		m_ForwardPassData = {};

		m_InstanceData.clear(); 
		m_InstanceDataBufferView.reset();

		m_InstanceDataOffsetBuffer.clear();
		m_InstanceDataOffsetBufferView.reset();

		m_MaterialDataBuffer.Clear();
		m_TraceRaysPipelineState.reset();

		m_PerSceneConstantBuffer = {};

		m_TotalInstanceCount = 0;
	}
}
