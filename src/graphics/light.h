#pragma once
#include "utils/types.h"

namespace vkr::Graphics
{
	enum LightType
	{
		LIGHT_TYPE_DIRECTIONAL,
		LIGHT_TYPE_POINT,
		LIGHT_TYPE_SPOT,

		LIGHT_TYPE_COUNT
	};

	struct Light
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