#include "viewrenderdata.h"

namespace vkr::Graphics
{
	void ViewRenderData::Clear()
	{
		m_VisibleMeshes.clear();
		m_VisibleLights.clear();
	}
}
