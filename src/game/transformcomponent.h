#pragma once
#include "component.h"
#include "core/types.h"

namespace vkr::Game
{
	struct TransformComponent
	{
		PROPERTY(Editable)
		Vector3f m_Position = Vector3f(0.0f, 0.0f, 0.0f);

		PROPERTY(Editable)
		Rotator m_Rotation;

		PROPERTY(Editable)
		Vector3f m_Scale = Vector3f(1.0f, 1.0f, 1.0f);
	};
}