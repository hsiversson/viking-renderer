#pragma once

#if ENABLE_EDITOR
#include "panel.h"
#include "graphics/view.h"
#include "graphics/camera.h"

namespace vkr::Graphics
{
	class Scene;
}

namespace vkr::Editor
{
	class EditorCameraController
	{
	public:
		EditorCameraController(Graphics::Camera& camera);
		void Update();
	private:
		Graphics::Camera& m_Camera;

		Vector3f m_CurrentVelocity;

		float m_MoveSpeed = 5.0f;
		float m_MouseSensitivity = 0.5f;
		float m_YawDeg = 0;
		float m_PitchDeg = 0;
	};

	class ViewportPanel final : public Panel
	{
	public:
		ViewportPanel(Graphics::Scene* scene);
		~ViewportPanel() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

	private:
		Graphics::Camera m_Camera;
		EditorCameraController m_CameraController;
		Graphics::View* m_View;
		Graphics::Scene* m_Scene;
		Graphics::TextureTarget m_ViewOutput;
	};
}

#endif //ENABLE_EDITOR