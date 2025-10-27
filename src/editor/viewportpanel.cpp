#include "viewportpanel.h"

#if ENABLE_EDITOR
#include "asset.h"
#include "editor.h"
#include "game/transformcomponent.h"
#include "game/modelcomponent.h"
#include "game/world.h"
#include "graphics/view.h"
#include "graphics/scene.h"
#include "core/inputmanager.h"

// TEMP
#include "graphics/modelloader_gltf.h"
// TEMP

#include "imguizmo.h"

namespace vkr::Editor
{
	EditorCameraController::EditorCameraController(Graphics::Camera& camera)
		: m_Camera(camera)
		, m_CurrentVelocity{0,0,0}
	{
	}

	void EditorCameraController::Update(bool isHovered)
	{
		ImGuiIO& io = ImGui::GetIO();
		Vector3f newVelocity = { 0,0,0 };
		if (isHovered && ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			ImVec2 delta = io.MouseDelta;
			Vector2f mouseMoveDelta = Vector2f(delta.x, delta.y);

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

			if (ImGui::IsKeyDown(ImGuiKey_W)) newVelocity = newVelocity + camForward;
			if (ImGui::IsKeyDown(ImGuiKey_S)) newVelocity = newVelocity - camForward;
			if (ImGui::IsKeyDown(ImGuiKey_A)) newVelocity = newVelocity - camRight;
			if (ImGui::IsKeyDown(ImGuiKey_D)) newVelocity = newVelocity + camRight;
			if (ImGui::IsKeyDown(ImGuiKey_Q)) newVelocity = newVelocity + camUp;
			if (ImGui::IsKeyDown(ImGuiKey_E)) newVelocity = newVelocity - camUp;

			if (Length(newVelocity) > 0.0f)
				newVelocity = Normalized(newVelocity);

			if (io.MouseWheel != 0.0f)
			{
				m_MoveSpeed += io.MouseWheel;
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

	bool ViewportWorldPicker::Init()
	{
		Render::Device* device = Render::GetDevice();
		Ref<Render::Shader> vertexShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/editor_writeobjectid.hlsl"), L"MainVS", Render::SHADER_STAGE_VERTEX);
		Ref<Render::Shader> pixelShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/editor_writeobjectid.hlsl"), L"MainPS", Render::SHADER_STAGE_PIXEL);
		
		Render::PipelineStateDesc psoDesc(Render::PIPELINE_STATE_TYPE_DEFAULT);
		psoDesc.Default.m_PrimitiveType = Render::PRIMITIVE_TYPE_TRIANGLE;
		psoDesc.Default.m_DepthStencilState.m_Enabled = true;
		psoDesc.Default.m_DepthStencilState.m_WriteDepth = true;
		psoDesc.Default.m_DepthStencilState.m_ComparisonFunc = Render::COMPARISON_FUNC_GREATER_EQUAL;
		psoDesc.Default.m_DepthStencilState.m_DSFormat = Render::FORMAT_D32_FLOAT;
		psoDesc.Default.m_RasterizerState.m_CullMode = Render::FACE_CULL_MODE_BACK;
		psoDesc.Default.m_RenderTargetState.m_Formats[0] = Render::FORMAT_RG32_UINT;
		psoDesc.Default.m_VertexShader = vertexShader.get();
		psoDesc.Default.m_PixelShader = pixelShader.get();
		m_WriteObjectIdPSO = device->CreatePipelineState(psoDesc);

		m_RenderTarget.m_Format = Render::FORMAT_RG32_UINT;
		m_RenderTarget.m_IsRenderTarget = true;

		m_DepthStencil.m_Format = Render::FORMAT_D32_FLOAT;
		m_DepthStencil.m_IsDepthStencil = true;

		return true;
	}

	bool ViewportWorldPicker::Run(const Vector2u& mousePosition, const Vector2u& viewportSize, Graphics::Camera& camera, const Game::World& world, std::vector<Game::Entity>& selectedEntities)
	{
		const Game::EntityRegistry& entityRegistry = world.GetEntityRegistry();
		auto modelComponents = entityRegistry.view<Game::ModelComponent>();

		std::vector<ObjectIdEntry> objects;
		for (Game::EntityHandle entityHandle : modelComponents)
		{
			const Game::ModelComponent& modelComponent = modelComponents.get<Game::ModelComponent>(entityHandle);
			const Game::TransformComponent& transformComponent = entityRegistry.get<Game::TransformComponent>(entityHandle);
			const std::vector<Graphics::Model::Part>& parts = modelComponent.m_Model->GetParts();
			for (const Graphics::Model::Part& part : parts)
			{
				FetchPartData(entityHandle, part, Compose(transformComponent.m_Position, transformComponent.m_Rotation.ToQuaternion(), transformComponent.m_Scale), objects);
			}
		}

		if (!objects.empty())
		{
			const Mat44 cameraViewProjection = camera.GetViewProjection();
			uint32_t targetIndex = ElapsedTimer::FrameIndex() % 3;

			m_LastWriteEvent = Render::QueueGraphicsTask([this, objects, viewportSize, cameraViewProjection, targetIndex]()
				{
					Render::Context* ctx = Render::Context::GetCurrentContext();
					ctx->ClearStateCache();

					m_RenderTarget.Update(viewportSize, "Object Id Buffer");
					m_DepthStencil.Update(viewportSize, "Object Id Depth");

					VKR_CONTEXT_EVENT(ctx, "Object Id");

					Render::TextureBarrierDesc barrier;
					barrier.m_Texture = m_RenderTarget.m_Texture.get();
					barrier.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
					barrier.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
					barrier.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
					ctx->TextureBarrier(barrier);

					ctx->ClearRenderTarget(m_RenderTarget.m_RenderTarget.get(), Vector4f(0.0f));
					ctx->ClearDepthStencil(m_DepthStencil.m_DepthStencil.get(), 0.0f);

					ctx->BindRenderTarget(m_RenderTarget.m_RenderTarget.get());
					ctx->BindDepthStencil(m_DepthStencil.m_DepthStencil.get());

					ctx->SetViewport(0, 0, viewportSize.x, viewportSize.y);
					ctx->SetScissorRect(0, 0, viewportSize.x, viewportSize.y);

					struct Constants
					{
						Mat44 ViewProjection;
						Mat44 ObjectTransform;
						uint32_t ObjectIdLowPart;
						uint32_t ObjectIdHighPart;
						uint32_t VertexBufferDescriptorIndex;
						uint32_t VertexPositionByteOffset;
						uint32_t VertexStride;
						uint32_t _pad[3];
					};

					ctx->BindPipelineState(m_WriteObjectIdPSO.get());
					for (const ObjectIdEntry& entry : objects)
					{
						Constants constants;
						constants.ViewProjection = cameraViewProjection;
						constants.ObjectTransform = entry.m_Transform;
						constants.ObjectIdLowPart = entry.m_ObjectIdLowPart;
						constants.ObjectIdHighPart = entry.m_ObjectIdHighPart;
						constants.VertexBufferDescriptorIndex = entry.m_VertexBuffer->GetIndex();
						constants.VertexPositionByteOffset = entry.m_PositionByteOffset;
						constants.VertexStride = entry.m_VertexStride;
						ctx->BindLocalConstantBuffer(sizeof(constants), &constants, 0);
						ctx->SetPrimitiveTopology(entry.m_Topology);
						ctx->BindIndexBuffer(entry.m_IndexBuffer);
						ctx->DrawIndexed(entry.m_IndexBuffer->GetDesc().m_ElementCount);
					}
				});
		}


		return false;
	}

	void ViewportWorldPicker::FetchPartData(const Game::EntityHandle entityHandle, const Graphics::Model::Part& part, const Mat44& parentWorldTransform, std::vector<ObjectIdEntry>& objects)
	{
		ObjectIdEntry entry = {};
		entry.m_VertexBuffer = part.m_Mesh->GetVertexBufferView().get();
		entry.m_IndexBuffer = part.m_Mesh->GetIndexBuffer().get();
		entry.m_Topology = part.m_Mesh->GetTopology();

		const Render::VertexLayout& vertexLayout = part.m_Mesh->GetVertexLayout();
		entry.m_PositionByteOffset = vertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_POSITION, 0);
		entry.m_VertexStride = vertexLayout.GetStride();

		entry.m_Transform = part.m_LocalTransform * parentWorldTransform;

		entry.m_ObjectIdHighPart = static_cast<uint32_t>((uint64_t)entityHandle >> 32);
		entry.m_ObjectIdLowPart = static_cast<uint32_t>(entityHandle);

		objects.push_back(entry);

		for (const Graphics::Model::Part& childPart : part.m_ChildParts)
		{
			FetchPartData(entityHandle, childPart, entry.m_Transform, objects);
		}
	}

	bool ViewportOutliner::Init()
	{
		Render::Device* device = Render::GetDevice();
		Ref<Render::Shader> vertexShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/editor_writeobjectoutline.hlsl"), L"MainVS", Render::SHADER_STAGE_VERTEX);
		Ref<Render::Shader> pixelShader = device->CreateShader(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/editor_writeobjectoutline.hlsl"), L"MainPS", Render::SHADER_STAGE_PIXEL);

		Render::PipelineStateDesc psoDesc(Render::PIPELINE_STATE_TYPE_DEFAULT);
		psoDesc.Default.m_PrimitiveType = Render::PRIMITIVE_TYPE_TRIANGLE;
		psoDesc.Default.m_DepthStencilState.m_Enabled = false;
		psoDesc.Default.m_RasterizerState.m_CullMode = Render::FACE_CULL_MODE_NONE;
		psoDesc.Default.m_RenderTargetState.m_Formats[0] = Render::FORMAT_RGBA16_FLOAT;
		psoDesc.Default.m_NumSamples = 8;
		psoDesc.Default.m_VertexShader = vertexShader.get();
		psoDesc.Default.m_PixelShader = pixelShader.get();
		m_WriteObjectOutlinePSO = device->CreatePipelineState(psoDesc);

		psoDesc.Default.m_RasterizerState.m_CullMode = Render::FACE_CULL_MODE_BACK;
		m_DiscardObjectPixelsPSO = device->CreatePipelineState(psoDesc);

		m_RenderTargetMS.m_IsRenderTarget = true;
		m_RenderTargetMS.m_Format = Render::FORMAT_RGBA16_FLOAT;
		m_RenderTargetMS.m_NumSamples = 8;

		m_ResolvedTarget.m_IsRenderTarget = true;
		m_ResolvedTarget.m_Format = Render::FORMAT_RGBA16_FLOAT;

		m_DepthStencilMS.m_IsDepthStencil = true;
		m_DepthStencilMS.m_Format = Render::FORMAT_D32_FLOAT;
		m_DepthStencilMS.m_NumSamples = 8;

		return true;
	}

	void ViewportOutliner::Run(const Vector2u& viewportSize, Graphics::Camera& camera, const std::vector<Game::Entity>& selectedEntities, const Game::World& world)
	{
		std::vector<OutlinerObject> outlineObjects;
		for (const Game::Entity& entity : selectedEntities)
		{
			if (entity.HasComponent<Game::ModelComponent>())
			{
				const Game::TransformComponent& transformComponent = entity.GetComponent<Game::TransformComponent>();
				const Game::ModelComponent& modelComponent = entity.GetComponent<Game::ModelComponent>();
				const std::vector<Graphics::Model::Part>& parts = modelComponent.m_Model->GetParts();
				for (const Graphics::Model::Part& part : parts)
				{
					FetchPartData(part, Compose(transformComponent.m_Position, transformComponent.m_Rotation.ToQuaternion(), transformComponent.m_Scale), outlineObjects);
				}
			}
		}

		if (!outlineObjects.empty())
		{
			const Mat44 cameraViewProjection = camera.GetViewProjection();
			const Mat44 cameraView = camera.GetView();

			Render::QueueGraphicsTask([this, outlineObjects, viewportSize, cameraViewProjection, cameraView]()
				{
					Render::Context* ctx = Render::Context::GetCurrentContext();
					ctx->ClearStateCache();

					m_RenderTargetMS.Update(viewportSize, "Outliner Target");
					m_DepthStencilMS.Update(viewportSize, "Outliner Depth");
					m_ResolvedTarget.Update(viewportSize, "Outliner Resolved Target");

					VKR_CONTEXT_EVENT(ctx, "Object Outline");

					Render::TextureBarrierDesc barrier;
					barrier.m_Texture = m_RenderTargetMS.m_Texture.get();
					barrier.m_TargetAccess = Render::RESOURCE_STATE_ACCESS_RENDER_TARGET;
					barrier.m_TargetLayout = Render::RESOURCE_STATE_LAYOUT_RENDER_TARGET;
					barrier.m_TargetSync = Render::RESOURCE_STATE_SYNC_RENDER_TARGET;
					ctx->TextureBarrier(barrier);

					ctx->ClearRenderTarget(m_RenderTargetMS.m_RenderTarget.get(), Vector4f(0.0f));
					ctx->ClearDepthStencil(m_DepthStencilMS.m_DepthStencil.get(), 0.0f);

					ctx->BindRenderTarget(m_RenderTargetMS.m_RenderTarget.get());
					ctx->BindDepthStencil(m_DepthStencilMS.m_DepthStencil.get());

					ctx->SetViewport(0, 0, viewportSize.x, viewportSize.y);
					ctx->SetScissorRect(0, 0, viewportSize.x, viewportSize.y);

					struct Constants
					{
						Mat44 ViewProjection;
						Mat44 View;
						Mat44 ObjectTransform;
						uint32_t VertexBufferDescriptorIndex;
						uint32_t VertexPositionByteOffset;
						uint32_t VertexNormalByteOffset;
						uint32_t VertexStride;
						Vector2f OutlineSizeNdc;
						float ColorIntensity;
						float _pad;
					};

					for (const OutlinerObject& obj : outlineObjects)
					{
						Constants constants = {};
						constants.ViewProjection = cameraViewProjection;
						constants.View = cameraView;
						constants.ObjectTransform = obj.m_Transform;
						constants.VertexBufferDescriptorIndex = obj.m_VertexBuffer->GetIndex();
						constants.VertexPositionByteOffset = obj.m_PositionByteOffset;
						constants.VertexNormalByteOffset = obj.m_NormalByteOffset;
						constants.VertexStride = obj.m_VertexStride;
						constants.ColorIntensity = 1.0f;
						constants.OutlineSizeNdc = Vector2f(1.0f / viewportSize.x, 1.0f / viewportSize.y) * 2.0f * 3.0f;
						ctx->BindLocalConstantBuffer(sizeof(constants), &constants, 0);
						ctx->SetPrimitiveTopology(obj.m_Topology);
						ctx->BindIndexBuffer(obj.m_IndexBuffer);
						ctx->BindPipelineState(m_WriteObjectOutlinePSO.get());
						ctx->DrawIndexed(obj.m_IndexBuffer->GetDesc().m_ElementCount);
					}

					for (const OutlinerObject& obj : outlineObjects)
					{
						Constants constants = {};
						constants.ViewProjection = cameraViewProjection;
						constants.View = cameraView;
						constants.ObjectTransform = obj.m_Transform;
						constants.VertexBufferDescriptorIndex = obj.m_VertexBuffer->GetIndex();
						constants.VertexPositionByteOffset = obj.m_PositionByteOffset;
						constants.VertexNormalByteOffset = obj.m_NormalByteOffset;
						constants.VertexStride = obj.m_VertexStride;
						constants.OutlineSizeNdc = Vector2f(0.0f);
						constants.ColorIntensity = 0.0f;
						ctx->BindLocalConstantBuffer(sizeof(constants), &constants, 0);
						ctx->SetPrimitiveTopology(obj.m_Topology);
						ctx->BindIndexBuffer(obj.m_IndexBuffer);
						ctx->BindPipelineState(m_DiscardObjectPixelsPSO.get());
						ctx->DrawIndexed(obj.m_IndexBuffer->GetDesc().m_ElementCount);
					}

					ctx->ResolveMultiSampleTarget(m_ResolvedTarget.m_Texture.get(), m_RenderTargetMS.m_Texture.get());
				});
		}
		else
		{
			if (m_ResolvedTarget.m_TextureView)
			{
				Render::QueueGraphicsTask([this, viewportSize]()
					{
						Render::Context* ctx = Render::Context::GetCurrentContext();
						m_ResolvedTarget.Update(viewportSize, "Outliner Resolved Target");
						ctx->ClearRenderTarget(m_ResolvedTarget.m_RenderTarget.get(), Vector4f(0.0f));
					});
			}
		}
	}

	Render::TextureView* ViewportOutliner::GetTexture() const
	{
		return m_ResolvedTarget.m_TextureView.get();
	}

	void ViewportOutliner::FetchPartData(const Graphics::Model::Part& part, const Mat44& parentWorldTransform, std::vector<OutlinerObject>& objects)
	{
		OutlinerObject obj = {};
		obj.m_VertexBuffer = part.m_Mesh->GetVertexBufferView().get();
		obj.m_IndexBuffer = part.m_Mesh->GetIndexBuffer().get();
		obj.m_Topology = part.m_Mesh->GetTopology();

		const Render::VertexLayout& vertexLayout = part.m_Mesh->GetVertexLayout();
		obj.m_PositionByteOffset = vertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_POSITION, 0);
		obj.m_NormalByteOffset = vertexLayout.GetByteOffset(Render::VertexAttribute::TYPE_NORMAL, 0);
		obj.m_VertexStride = vertexLayout.GetStride();

		obj.m_Transform = part.m_LocalTransform * parentWorldTransform;

		objects.push_back(obj);

		for (const Graphics::Model::Part& childPart : part.m_ChildParts)
		{
			FetchPartData(childPart, obj.m_Transform, objects);
		}
	}

    ViewportPanel::ViewportPanel(Game::World& world)
        : Panel("Viewport")
        , m_World(world)
		, m_CameraController(m_Camera)
        , m_View(nullptr)
		, m_IsHovered(false)
    {
        m_View = m_World.GetGraphicsScene()->CreateView();

		m_ViewOutput.m_Format = Render::FORMAT_RGBA16_FLOAT;
		m_ViewOutput.m_IsWritable = true;
		m_ViewOutput.m_IsRenderTarget = true;
		m_ViewOutput.Update(1280, 720, "Editor Viewport Target");
		m_View->SetOutputTarget(m_ViewOutput.m_RenderTarget.get());
		m_View->SetRenderSize(Vector2u(1280, 720));
		m_View->SetOutputSize(Vector2u(1280, 720));

		Mat43 camTransform = Compose(Mat33::Identity(), Vector3f(0, 2.0f, -6.0f));
		m_Camera.SetLocalTransform(camTransform);
		m_Camera.SetSize(Vector2f(1280, 720));
		m_Camera.SetInvertedZ(true);
		m_View->SetCamera(m_Camera);

		m_WorldPicker.Init();
		m_Outliner.Init();
    }

    ViewportPanel::~ViewportPanel()
	{
		m_World.GetGraphicsScene()->DestroyView(m_View);
    }

    void ViewportPanel::OnUpdate()
	{
		Vector2u viewportSize = Vector2u(m_ContentAreaSize);
		viewportSize.x += (viewportSize.x & 1);
		viewportSize.y += (viewportSize.y & 1);

        m_Camera.SetSize(Vector2f(viewportSize));
		m_ViewOutput.Update(viewportSize, "Editor Viewport Target");
		m_View->SetOutputTarget(m_ViewOutput.m_RenderTarget.get());
		m_View->SetOutputSize(viewportSize);

		m_CameraController.Update(m_IsHovered);

        m_View->SetCamera(m_Camera);

		//if (m_PickRequested)
		{
			std::vector<Game::Entity> selectedEntities;
			{
				m_WorldPicker.Run({0,0}, viewportSize, m_Camera, m_World, selectedEntities);
			}
		}
		if (!m_SelectedEntities.empty())
		{
			m_Outliner.Run(viewportSize, m_Camera, m_SelectedEntities, m_World);
		}
    }

    void ViewportPanel::OnDraw()
    {
		auto OutputTexture = m_View->GetRenderTargets().m_SceneBuffer_OutputSize.m_Texture;
        Vector2f uvMax = { m_ContentAreaSize.x / (float)m_ViewOutput.m_Texture->m_TextureDesc.m_Size.x, m_ContentAreaSize.y / (float)m_ViewOutput.m_Texture->m_TextureDesc.m_Size.y };

		ImVec2 curPos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)m_ViewOutput.m_TextureView.get(), { m_ContentAreaSize.x, m_ContentAreaSize.y }, { 0,0 }, { uvMax.x, uvMax.y });

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_AssetDragSource", ImGuiDragDropFlags_None))
			{
				AssetDragDropPayload unpacked = {};
				memcpy(&unpacked, payload->Data, payload->DataSize);
				HandleAssetDrop(unpacked);
			}

			ImGui::EndDragDropTarget();
		}

		Render::TextureView* outlinerTexture = m_Outliner.GetTexture();
		if (!m_SelectedEntities.empty() && outlinerTexture)
		{
			ImGui::SetCursorScreenPos(curPos);
			ImGui::Image((ImTextureID)m_Outliner.GetTexture(), { m_ContentAreaSize.x, m_ContentAreaSize.y }, { 0,0 }, { uvMax.x, uvMax.y });
		}
		m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

		if (!m_SelectedEntities.empty())
		{
			// TODO: handle multiple entities
			//		 build a transform that positions itself in the middle of all of the entities

			if (ImGui::IsKeyPressed(ImGuiKey_1))
			{
				m_SelectedGizmoOp = GizmoOperation::Translate;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_2))
			{
				m_SelectedGizmoOp = GizmoOperation::Rotate;
			}
			else if (ImGui::IsKeyPressed(ImGuiKey_3))
			{
				m_SelectedGizmoOp = GizmoOperation::Scale;
			}

			Game::TransformComponent& transformComponent = m_SelectedEntities[0].GetComponent<Game::TransformComponent>();

			Mat44 cameraTransform = m_Camera.GetWorldTransform();

			Mat44 objectTransform = Compose(transformComponent.m_Position, transformComponent.m_Rotation.ToQuaternion(), transformComponent.m_Scale);
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

				const IMGUIZMO_NAMESPACE::OPERATION op = (m_SelectedGizmoOp == GizmoOperation::Scale) ? IMGUIZMO_NAMESPACE::SCALE : (m_SelectedGizmoOp == GizmoOperation::Rotate ? IMGUIZMO_NAMESPACE::ROTATE : IMGUIZMO_NAMESPACE::TRANSLATE);
				const IMGUIZMO_NAMESPACE::MODE mode = m_SelectedGizmoSpace == GizmoSpace::Local ? IMGUIZMO_NAMESPACE::LOCAL : IMGUIZMO_NAMESPACE::WORLD;
				if (ImGuizmo::Manipulate(&view[0], &proj[0], op, mode, &objectTransform[0]))
				{
					Quaternion rotation;
					Decompose(objectTransform, transformComponent.m_Position, rotation, transformComponent.m_Scale);
					transformComponent.m_Rotation.FromQuaternion(rotation);
				}
				ImGui::PopClipRect();
			}
		}
    }

	void ViewportPanel::ReceiveMessage(const BroadcastMessage& message)
	{
		if (message.GetId() == BROADCAST_MSG_ID_SELECTED_ENTITIES)
		{
			struct Selection
			{
				uint32_t m_NumEntities;
				Game::Entity* m_Entities;
			};
			Selection selection;
			message.GetData(selection);

			m_SelectedEntities.clear();
			if (selection.m_NumEntities > 0)
			{
				m_SelectedEntities.insert(m_SelectedEntities.end(), selection.m_Entities, selection.m_Entities + selection.m_NumEntities);
			}
		}
	}

	bool ViewportPanel::HandleAssetDrop(const AssetDragDropPayload& payload)
	{
		const std::filesystem::path path = payload.m_Path;
		if (path.extension() == ".gltf")
		{
			const std::string name = path.stem().string();

			Game::Entity entity = m_World.CreateEntity(name.c_str());

			Mat43 cameraTransform = m_Camera.GetWorldTransform();
			Vector3f cameraPosition = Vector3f(cameraTransform.At(3, 0), cameraTransform.At(3, 1), cameraTransform.At(3, 2));
			Vector3f placementPosition = cameraPosition + Normalized(Vector3f(cameraTransform.At(2, 0), cameraTransform.At(2, 1), cameraTransform.At(2, 2))) * 3.0f;
			Game::TransformComponent& transformComponent = entity.AddComponent<Game::TransformComponent>();
			transformComponent.m_Position = placementPosition;

			Game::ModelComponent& modelComponent = entity.AddComponent<Game::ModelComponent>();
			modelComponent.m_ModelFilePath = path;

			Graphics::ModelLoader_GLTF loader;
			modelComponent.m_Model = loader.Load(modelComponent.m_ModelFilePath);
			m_World.GetGraphicsScene()->AddModel(modelComponent.m_Model);

			BroadcastMessage msg(BROADCAST_MSG_ID_SELECTED_ENTITIES);
			struct Selection
			{
				uint32_t m_NumEntities;
				Game::Entity* m_Entities;
			};
			Selection selection = {};
			selection.m_NumEntities = 1;
			selection.m_Entities = &entity;
			msg.SetData(selection);
			Manager::Get()->Broadcast(msg);
		}

		return false;
	}
}

#endif //ENABLE_EDITOR