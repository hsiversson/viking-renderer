#pragma once
#include "component.h"
#include "core/types.h"
#include "core/property.h"

namespace vkr::Game
{
	struct TransformComponent : public IComponent
	{
		Vector3f m_Position = Vector3f(0.0f, 0.0f, 0.0f);
		Quaternion m_Rotation = Quaternion::Identity();
		Vector3f m_Scale = Vector3f(1.0f, 1.0f, 1.0f);
	};
}