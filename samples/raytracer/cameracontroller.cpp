#include "cameracontroller.h"

#include "core/inputmanager.h"
#include "core/types.h"

#include <algorithm>

using namespace vkr;

void CameraController::Init(Ref<Graphics::Camera> camera, InputManager* inputManager)
{
	m_InputManager = inputManager;
	m_Camera = camera;
}

void CameraController::Tick(float deltaTime)
{

	Vector2f rotdelta = m_InputManager->GetMouseDelta();
	m_YawDeg += rotdelta.x * m_MouseSensitivity;
	m_YawDeg = std::fmod(m_YawDeg, 360.0f);
	if (m_YawDeg < 0.0f)
		m_YawDeg += 360.0f;
	m_PitchDeg -= rotdelta.y * m_MouseSensitivity;
	m_PitchDeg = std::clamp(m_PitchDeg, -89.0f, 89.0f);

	float pitchRad = DEG_TO_RAD(m_PitchDeg);
	float yawRad = DEG_TO_RAD(m_YawDeg);

	//Build final transform of camera
	Vector3f camForward = Normalized(Vector3f(
		cos(pitchRad) * sin(yawRad),
		sin(pitchRad),
		cos(pitchRad) * cos(yawRad)
	));

	Vector3f camRight = Normalized(Cross(Vector3f(0, 1, 0),camForward));
	Vector3f camUp = Normalized(Cross(camForward,camRight));

	Vector3f movement = {0,0,0};

	if (m_InputManager->IsPressed(vkr::KeyW)) movement = movement + camForward;
	if (m_InputManager->IsPressed(vkr::KeyS)) movement = movement - camForward;
	if (m_InputManager->IsPressed(vkr::KeyA)) movement = movement - camRight;
	if (m_InputManager->IsPressed(vkr::KeyD)) movement = movement + camRight;
	if (m_InputManager->IsPressed(vkr::KeyQ)) movement = movement + camUp;
	if (m_InputManager->IsPressed(vkr::KeyE)) movement = movement - camUp;

	if (Length(movement) > 0.0f)
		movement = Normalized(movement);

	Mat43 oldTransform = m_Camera->GetLocalTransform();
	Vector3f oldTranslation = Vector3f(oldTransform[9], oldTransform[10], oldTransform[11]);

	movement = oldTranslation + movement * m_MoveSpeed * deltaTime;

	Mat43 localTransform = {
		camRight.x,camRight.y,camRight.z,
		camUp.x,camUp.y,camUp.z,
		camForward.x,camForward.y,camForward.z,
		movement.x,movement.y,movement.z
	};
	m_Camera->SetLocalTransform(localTransform);
}
