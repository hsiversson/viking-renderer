#pragma once
#include "component.h"
#include "core/types.h"
#include "core/property.h"

namespace vkr::Game
{
	struct TransformComponent : public IComponent
	{
		PROPERTY(Vector3f, Position);
		PROPERTY(Quaternion, Rotation);
		PROPERTY(Vector3f, Scale, Vector3f(1.0f, 1.0f, 1.0f));
	};
}