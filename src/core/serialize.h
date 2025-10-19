#pragma once

#include "nlohmann/json.hpp"

namespace vkr
{
	using Json = nlohmann::json;

	struct ISerializable
	{
		virtual ~ISerializable() = default;
		virtual void Serialize(Json& data) const = 0;
		virtual void Deserialize(const Json& data) = 0;
	};
}