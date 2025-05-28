#pragma once
#include "light.h"

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
			if (m_Material < other.m_Material)
				return true;
			else if (m_Mesh < other.m_Mesh)
				return true;
			else if (m_DistanceToCamera < other.m_DistanceToCamera)
				return true;
			return false;
		}
	};

	struct ViewRenderData
	{
		void Clear();

		std::vector<RenderObject> m_VisibleMeshes;
		std::vector<Light> m_VisibleLights;
	};
}