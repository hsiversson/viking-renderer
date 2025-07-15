#pragma once

#include "tickable.h"
#include "graphics/camera.h"

namespace vkr
{
	class InputManager;
}

class CameraController : public ITickable
{
public:
	CameraController() {}
	void Init(vkr::Ref<vkr::Graphics::Camera> camera, vkr::InputManager* inputManager);
	void Tick(float deltaTime) override;

private:
	float m_MoveSpeed = 5.0f;
	float m_MouseSensitivity = 0.5f;
	float m_YawDeg = 0;
	float m_PitchDeg = 0;
	vkr::InputManager* m_InputManager = nullptr;
	vkr::Ref<vkr::Graphics::Camera> m_Camera;
};