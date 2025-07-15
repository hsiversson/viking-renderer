#pragma once

#include "application/application.h"
#include "core/types.h"
#include "tickable.h"

namespace vkr::Graphics
{
	class Camera;
}

class RaytracerApp : public vkr::Application 
{
public:
	void AppInit() override;
	void Tick(float deltaTime) override;
private:
	vkr::Ref<vkr::Graphics::Camera> m_Camera;
	std::vector<vkr::Ref<ITickable>> m_Tickables;
};