#include "viewportpanel.h"

#if ENABLE_EDITOR
#include "editor.h"
#include "graphics/view.h"
#include "graphics/scene.h"
#include "core/inputmanager.h"

#include "imguizmo.h"

namespace vkr::Editor
{
	EditorCameraController::EditorCameraController(Graphics::Camera& camera)
		: m_Camera(camera)
		, m_CurrentVelocity{0,0,0}
	{
	}

	void EditorCameraController::Update()
	{
		InputManager* inputManager = Manager::Get()->GetInputManager();

		Vector3f newVelocity = { 0,0,0 };
		if (inputManager->IsMouseKeyPressed(INPUT_MOUSE_KEY_RIGHT))
		{
			Vector2f mouseMoveDelta = Vector2f(inputManager->GetMouseDelta());

			m_YawDeg += mouseMoveDelta.x * m_MouseSensitivity;
			m_YawDeg = std::fmod(m_YawDeg, 360.0f);
			if (m_YawDeg < 0.0f)
				m_YawDeg += 360.0f;
			m_PitchDeg -= mouseMoveDelta.y * m_MouseSensitivity;
			m_PitchDeg = std::clamp(m_PitchDeg, -89.0f, 89.0f);

			float pitchRad = DegToRad(m_PitchDeg);
			float yawRad = DegToRad(m_YawDeg);

			//Build final transform of camera
			Vector3f camForward = Normalized(Vector3f(
				cos(pitchRad) * sin(yawRad),
				sin(pitchRad),
				cos(pitchRad) * cos(yawRad)
			));

			Vector3f camRight = Normalized(Cross(Vector3f(0, 1, 0), camForward));
			Vector3f camUp = Normalized(Cross(camForward, camRight));

			if (inputManager->IsKeyPressed(INPUT_KEY_W)) newVelocity = newVelocity + camForward;
			if (inputManager->IsKeyPressed(INPUT_KEY_S)) newVelocity = newVelocity - camForward;
			if (inputManager->IsKeyPressed(INPUT_KEY_A)) newVelocity = newVelocity - camRight;
			if (inputManager->IsKeyPressed(INPUT_KEY_D)) newVelocity = newVelocity + camRight;
			if (inputManager->IsKeyPressed(INPUT_KEY_Q)) newVelocity = newVelocity + camUp;
			if (inputManager->IsKeyPressed(INPUT_KEY_E)) newVelocity = newVelocity - camUp;

			if (Length(newVelocity) > 0.0f)
				newVelocity = Normalized(newVelocity);

			if (inputManager->GetMouseScrollDelta() != 0.0f)
			{
				m_MoveSpeed += inputManager->GetMouseScrollDelta() * 100.0f;
				m_MoveSpeed = std::clamp(m_MoveSpeed, 0.01f, 50.0f);
			}

			Mat43 prevTransform = m_Camera.GetLocalTransform();
			Vector3f prevPosition = { prevTransform.At(3, 0), prevTransform.At(3, 1), prevTransform.At(3, 2) };
			if (Length(newVelocity - m_CurrentVelocity) < 0.01f)
			{
				m_CurrentVelocity = newVelocity;
			}
			else
			{
				m_CurrentVelocity = Lerp(m_CurrentVelocity, newVelocity, 5.0f * ElapsedTimer::DeltaTime());
			}

			Vector3f newPosition = prevPosition + m_CurrentVelocity * m_MoveSpeed * ElapsedTimer::DeltaTime();

			Mat43 localTransform = {
				camRight.x,camRight.y,camRight.z,
				camUp.x,camUp.y,camUp.z,
				camForward.x,camForward.y,camForward.z,
				newPosition.x,newPosition.y,newPosition.z
			};
			m_Camera.SetLocalTransform(localTransform);
		}
		else if (Length(m_CurrentVelocity) > 0.0f)
		{
			if (Length(newVelocity - m_CurrentVelocity) < 0.01f)
			{
				m_CurrentVelocity = newVelocity;
			}
			else
			{
				m_CurrentVelocity = Lerp(m_CurrentVelocity, newVelocity, 5.0f * ElapsedTimer::DeltaTime());
			}
			Mat43 localTransform = m_Camera.GetLocalTransform();
			Vector3f prevPosition = { localTransform.At(3, 0), localTransform.At(3, 1), localTransform.At(3, 2) };
			
			Vector3f newPosition = prevPosition + m_CurrentVelocity * m_MoveSpeed * ElapsedTimer::DeltaTime();

			localTransform.At(3, 0) = newPosition.x;
			localTransform.At(3, 1) = newPosition.y;
			localTransform.At(3, 2) = newPosition.z;

			m_Camera.SetLocalTransform(localTransform);
		}
	}

    ViewportPanel::ViewportPanel(Graphics::Scene* scene)
        : Panel("Viewport")
		, m_CameraController(m_Camera)
        , m_View(nullptr)
        , m_Scene(scene)
    {
        m_View = m_Scene->CreateView();

        m_ViewOutput.m_Format = Render::FORMAT_RGBA16_FLOAT;
        m_ViewOutput.m_IsWritable = true;
        m_ViewOutput.m_IsRenderTarget = true;
        m_ViewOutput.Update(1280, 720, "Editor Viewport Target");
		m_View->SetOutputTarget(m_ViewOutput.m_RenderTarget.get());
		m_View->SetRenderSize(Vector2u(1280, 720));

		Mat43 camTransform = Compose(Mat33::Identity(), Vector3f(0, 2.0f, -6.0f));
		m_Camera.SetLocalTransform(camTransform);
		m_Camera.SetSize(Vector2f(1280, 720));
		m_Camera.SetInvertedZ(true);
		m_View->SetCamera(m_Camera);
    }

    ViewportPanel::~ViewportPanel()
	{
		m_Scene->DestroyView(m_View);
    }

    void ViewportPanel::OnUpdate()
	{
		Vector2u viewportSize = Vector2u(m_ContentAreaSize);
		viewportSize.x += (viewportSize.x & 1);
		viewportSize.y += (viewportSize.y & 1);

        m_Camera.SetSize(Vector2f(viewportSize));
		m_ViewOutput.Update(viewportSize, "Editor Viewport Target");
		m_View->SetOutputTarget(m_ViewOutput.m_RenderTarget.get());
		m_View->SetRenderSize(viewportSize);

		m_CameraController.Update();

        m_View->SetCamera(m_Camera);
    }

    void ViewportPanel::OnDraw()
    {
        Vector2f uvMax = { m_ContentAreaSize.x / (float)m_ViewOutput.m_Texture->m_TextureDesc.m_Size.x, m_ContentAreaSize.y / (float)m_ViewOutput.m_Texture->m_TextureDesc.m_Size.y };
        ImGui::Image((ImTextureID)m_ViewOutput.m_TextureView.get(), { m_ContentAreaSize.x, m_ContentAreaSize.y }, { 0,0 }, { uvMax.x, uvMax.y });

		bool selected = true;
		if (selected)
		{
			Mat44 cameraTransform = m_Camera.GetWorldTransform();

			Mat44 objectTransform = Mat44::Identity();
			Vector3f objectPosition = Vector3f(objectTransform.At(3, 0), objectTransform.At(3, 1), objectTransform.At(3, 2));

			Vector3f cameraForward = Normalized(Vector3f(cameraTransform.At(2, 0), cameraTransform.At(2, 1), cameraTransform.At(2, 2)));
			Vector3f cameraPosition = Vector3f(cameraTransform.At(3, 0), cameraTransform.At(3, 1), cameraTransform.At(3, 2));
			Vector3f toObject = Normalized(objectPosition - cameraPosition);

			if (Dot(cameraForward, toObject) > 0.0f)
			{
				ImGui::PushClipRect({ m_ContentAreaPosition.x, m_ContentAreaPosition.y }, { m_ContentAreaPosition.x + m_ContentAreaSize.x, m_ContentAreaPosition.y + m_ContentAreaSize.y }, false);
				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
				ImGuizmo::SetRect(m_ContentAreaPosition.x, m_ContentAreaPosition.y, m_ContentAreaSize.x, m_ContentAreaSize.y);

				Mat44 view = m_Camera.GetView();
				Mat44 proj = m_Camera.GetProjection();
				ImGuizmo::Manipulate(&view[0], &proj[0], IMGUIZMO_NAMESPACE::TRANSLATE, IMGUIZMO_NAMESPACE::WORLD, &objectTransform[0]);
				ImGui::PopClipRect();
			}
		}
    }
}

#endif //ENABLE_EDITOR