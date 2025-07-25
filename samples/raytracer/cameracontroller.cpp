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
	if (m_InputManager->IsMouseKeyPressed(vkr::INPUT_MOUSE_KEY_RIGHT))
	{
		Vector2f rotdelta = (Vector2f)m_InputManager->GetMouseDelta();
		m_YawDeg += rotdelta.x * m_MouseSensitivity;
		m_YawDeg = std::fmod(m_YawDeg, 360.0f);
		if (m_YawDeg < 0.0f)
			m_YawDeg += 360.0f;
		m_PitchDeg -= rotdelta.y * m_MouseSensitivity;
		m_PitchDeg = std::clamp(m_PitchDeg, -89.0f, 89.0f);
	}

	float pitchRad = DegToRad(m_PitchDeg);
	float yawRad = DegToRad(m_YawDeg);

	//Build final transform of camera
	Vector3f camForward = Normalized(Vector3f(
		cos(pitchRad) * sin(yawRad),
		sin(pitchRad),
		cos(pitchRad) * cos(yawRad)
	));

	Vector3f camRight = Normalized(Cross(Vector3f(0, 1, 0),camForward));
	Vector3f camUp = Normalized(Cross(camForward,camRight));

	Vector3f movement = {0,0,0};

	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_W)) movement = movement + camForward;
	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_S)) movement = movement - camForward;
	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_A)) movement = movement - camRight;
	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_D)) movement = movement + camRight;
	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_Q)) movement = movement + camUp;
	if (m_InputManager->IsKeyPressed(vkr::INPUT_KEY_E)) movement = movement - camUp;

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
