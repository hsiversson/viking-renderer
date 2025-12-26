#pragma once

#if ENABLE_EDITOR
#include "broadcast.h"
#include "panel.h"
#include "game/entity.h"
#include "graphics/view.h"
#include "graphics/camera.h"
#include "graphics/model.h"

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

	class ViewportWorldPicker
	{
	public:
		bool Init();
		bool Run(const Vector2u& mousePosition, const Vector2u& viewportSize, Graphics::Camera& camera, const Game::World& world, std::vector<Game::Entity>& selectedEntities);

		bool GetSelection(const Vector2u& mousePosition, const Vector2u& viewportSize, Game::World& world, std::vector<Game::Entity>& selectedEntities);

	private:
		struct ObjectIdEntry
		{
			uint32_t m_ObjectIdLowPart;
			uint32_t m_ObjectIdHighPart;
			Render::BufferView* m_VertexBuffer;
			Render::Buffer* m_IndexBuffer;
			Render::PrimitiveTopology m_Topology;
			uint32_t m_PositionByteOffset;
			uint32_t m_VertexStride;
			Mat44 m_Transform;
		};

		void FetchPartData(const Game::EntityHandle entityHandle, const Graphics::Model::Part& part, const Mat44& parentWorldTransform, std::vector<ObjectIdEntry>& objects);

		Ref<Render::RenderTaskEvent> m_LastWriteEvent;
		Ref<Render::PipelineState> m_ClearObjectIdPSO;
		Ref<Render::PipelineState> m_WriteObjectIdPSO;
		Graphics::TextureTarget m_RenderTarget;
		std::array<std::vector<Game::EntityHandle>, 3> m_ResolvedTargets;
		Graphics::TextureTarget m_DepthStencil;
	};

	class ViewportOutliner
	{
	public:
		bool Init();
		void Run(const Vector2u& viewportSize, Graphics::Camera& camera, const std::vector<Game::Entity>& selectedEntities, const Game::World& world);

		Render::TextureView* GetTexture() const;

	private:
		struct OutlinerObject
		{
			Render::BufferView* m_VertexBuffer;
			Render::Buffer* m_IndexBuffer;
			Render::PrimitiveTopology m_Topology;
			uint32_t m_PositionByteOffset;
			uint32_t m_NormalByteOffset;
			uint32_t m_VertexStride;
			Mat44 m_Transform;
		};

		void FetchPartData(const Graphics::Model::Part& part, const Mat44& parentWorldTransform, std::vector<OutlinerObject>& objects);

		Ref<Render::PipelineState> m_WriteObjectOutlinePSO;
		Ref<Render::PipelineState> m_DiscardObjectPixelsPSO;
		Graphics::TextureTarget m_RenderTargetMS;
		Graphics::TextureTarget m_DepthStencilMS;
		Graphics::TextureTarget m_ResolvedTarget;
		bool m_TargetCleared;
	};

	struct AssetDragDropPayload;
	class ViewportPanel final : public Panel, public BroadcastListener
	{
	public:
		ViewportPanel(Game::World& world);
		~ViewportPanel() override;

	private:
		void OnUpdate() override;
		void OnDraw() override;

		void ReceiveMessage(const BroadcastMessage& message) override;

		bool HandleAssetDrop(const AssetDragDropPayload& payload);

	private:
		Game::World& m_World;
		Graphics::Camera m_Camera;
		EditorCameraController m_CameraController;
		ViewportWorldPicker m_WorldPicker;
		ViewportOutliner m_Outliner;
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
		std::vector<Game::Entity> m_SelectedEntities;
		GizmoOperation m_SelectedGizmoOp = GizmoOperation::Translate;
		GizmoSpace m_SelectedGizmoSpace = GizmoSpace::World;

		bool m_IsHovered;
	};
}

#endif //ENABLE_EDITOR