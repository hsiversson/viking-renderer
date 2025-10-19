#pragma once

#include "core/serialize.h"

namespace vkr::Game
{
	class IComponent : public ISerializable
	{
	public:
		virtual void OnComponentAdded() {}
		virtual void OnComponentRemoved() {}

		virtual void Serialize(Json&) const {}
		virtual void Deserialize(const Json&) {}
	};
}