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

		void Serialize(Json& s) const
		{
			s["position"] = { m_Position.x, m_Position.y, m_Position.z };
			s["rotation"] = { m_Rotation.w, m_Rotation.x, m_Rotation.y, m_Rotation.z };
			s["scale"] = { m_Scale.x, m_Scale.y, m_Scale.z };
		}

		void Deserialize(const Json& s)
		{
			const Json& position = s.at("position");
			m_Position = Vector3f(position[0], position[1], position[2]);

			const Json& rotation = s.at("rotation");
			m_Rotation = Quaternion(rotation[0], rotation[1], rotation[2], rotation[3]);

			const Json& scale = s.at("scale");
			m_Scale = Vector3f(scale[0], scale[1], scale[2]);
		}
	};
}