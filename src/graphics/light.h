#pragma once
#include "core/types.h"

namespace vkr::Graphics
{
	struct DirectionalLight
	{
		Vector3f Emission;
		float Radius;
		Vector3f Direction;
		float _unused;
	};

	enum LocalLightType : uint8_t
	{
		LOCAL_LIGHT_TYPE_POINT,
		LOCAL_LIGHT_TYPE_SPOT,
		LOCAL_LIGHT_TYPE_COUNT
	};

	struct LocalLight
	{
		Vector3f m_Position;
		float m_Range;

		Vector3f m_Direction;
		float m_SourceSize;

		Vector3f m_Emission;
		uint32_t m_Type;

		Vector2f m_ConeAngles;
		float _unused[2];
	};
}