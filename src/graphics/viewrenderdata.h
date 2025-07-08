#pragma once
#include "light.h"
#include "render/device.h"

namespace vkr::Graphics
{
	class Mesh;
	class Material;

	struct RenderObject
	{
		Mesh* m_Mesh;
		Material* m_Material;
		Mat44 m_Transform;
		float m_DistanceToCamera;

		// State-sort operator material->mesh->distance
		bool operator<(const RenderObject& other) const
		{
			if (m_Material != other.m_Material)
				return m_Material < other.m_Material;

			if (m_Mesh != other.m_Mesh)
				return m_Mesh < other.m_Mesh;

			return m_DistanceToCamera < other.m_DistanceToCamera;
		}
	};

	struct ViewRenderData
	{
		void Clear();

		Ref<Render::BufferView> m_RaytracingTLAS;

		std::vector<Render::RaytracingInstanceDesc> m_RaytracingInstances;
		std::vector<RenderObject> m_VisibleMeshes;
		std::vector<Light> m_VisibleLights;
	};
}