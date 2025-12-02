#pragma once
#include "component.h"
#include "core/types.h"
#include "graphics/light.h"

namespace vkr::Game
{
	struct DirectionalLightComponent : public IComponent
	{
		Ref<Graphics::DirectionalLight> m_Light = MakeRef<Graphics::DirectionalLight>();

		PROPERTY(editable, min=0.0, default=(1.0, 1.0, 1.0))
		Vector3f m_Color = Vector3f(1.0f, 1.0f, 1.0f);

		PROPERTY(editable, min=0.0, step=0.1, default = 6.0)
		float m_Intensity = 6.0f;

		PROPERTY(editable, min=0.0, max=10.0, step=0.01, default=0.27)
		float m_Radius = 0.27f; //Sun angular diameter is 0.5357 deg
	};

	struct LocalLightComponent : public IComponent
	{
		Graphics::LocalLightType m_Type;
		Ref<Graphics::LocalLight> m_Light;

		void OnComponentAdded() override
		{
			m_Light = MakeRef<Graphics::LocalLight>();
		}
	};
}