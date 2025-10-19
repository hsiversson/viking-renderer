#pragma once
#include "component.h"
#include "core/types.h"
#include "core/property.h"
#include "graphics/light.h"

namespace vkr::Game
{
	struct DirectionalLightComponent : public IComponent
	{
		Ref<Graphics::DirectionalLight> m_Light;
		Vector3f m_Color = Vector3f(1.0f, 1.0f, 1.0f);
		float m_Intensity = 3.0f;
		float m_Radius = 0.5357f;

		void OnComponentAdded() override
		{
			m_Light = MakeRef<Graphics::DirectionalLight>();
		}

		void Serialize(Json& s) const
		{
			s["color"] = { m_Color.x, m_Color.y, m_Color.z };
			s["intensity"] = m_Intensity;
			s["radius"] = m_Radius;
		}

		void Deserialize(const Json& s)
		{
			const Json& c = s.at("color");
			m_Color = Vector3f(c[0], c[1], c[2]);

			m_Intensity = s.at("intensity");
			m_Radius = s.at("radius");
		}
	};

	struct LocalLightComponent : public IComponent
	{
		Graphics::LocalLightType m_Type;
		Ref<Graphics::LocalLight> m_Light;

		void OnComponentAdded() override
		{
			m_Light = MakeRef<Graphics::LocalLight>();
		}

		void Serialize(Json& s) const
		{
			s["type"] = m_Type;
		}

		void Deserialize(const Json& s)
		{
			m_Type = s.at("type");
		}
	};
}