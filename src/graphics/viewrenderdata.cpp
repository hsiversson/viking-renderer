#include "viewrenderdata.h"

namespace vkr::Graphics
{
	void ViewRenderData::Clear()
	{
		m_RaytracingInstances.clear();
		m_VisibleMeshes.clear();
		m_VisibleLights.clear();
		m_DepthPassData = {};
		m_ForwardPassData = {};
		m_InstanceData.clear();
		m_InstanceDataOffsetBuffer.clear();
	}
}
