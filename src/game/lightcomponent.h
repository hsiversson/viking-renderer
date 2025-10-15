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
			m_Light->Direction = Normalized(Vector3f(0.0f, 0.45f, 1.0f));
			m_Light->Emission = m_Color * m_Intensity;
			m_Light->Radius = DegToRad(m_Radius);
		}
	};

	struct LocalLightComponent : public IComponent
	{
		Graphics::LocalLightType m_Type;
		Ref<Graphics::LocalLight> m_Light;
	};
}