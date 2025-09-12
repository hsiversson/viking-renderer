#pragma once

#if ENABLE_EDITOR
#include "broadcast.h"
#include "panel.h"
#include "game/entity.h"
#include "graphics/view.h"
#include "graphics/camera.h"

namespace vkr::Game
{
	class World;
}

namespace vkr::Editor
{
	class EditorCameraController
	{
	public:
		EditorCameraController(Graphics::Camera& camera);
		void Update(bool isHovered);

	private:
		Graphics::Camera& m_Camera;

		Vector3f m_CurrentVelocity;

		float m_MoveSpeed = 5.0f;
		float m_MouseSensitivity = 0.5f;
		float m_YawDeg = 0;
		float m_PitchDeg = 0;
	};

	class ViewportPanel final : public Panel, public BroadcastListener
	{
	public:
		ViewportPanel(Game::World& world);
		~ViewportPanel() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

		void ReceiveMessage(const BroadcastMessage& message) override;

	private:
		Game::World& m_World;
		Graphics::Camera m_Camera;
		EditorCameraController m_CameraController;
		Graphics::View* m_View;
		Graphics::TextureTarget m_ViewOutput; 

		//Gizmo controls
		enum class GizmoOperation
		{
			Translate,
			Rotate,
			Scale 
		};
		enum class GizmoSpace
		{
			World,
			Local
		};
		Game::Entity m_SelectedEntity;
		GizmoOperation m_SelectedGizmoOp = GizmoOperation::Rotate;
		GizmoSpace m_SelectedGizmoSpace = GizmoSpace::World;

		bool m_IsHovered;
	};
}

#endif //ENABLE_EDITOR