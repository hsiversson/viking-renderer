#include "application.h"
#include "core/commandline.h"

#include "render/device.h"
#include "render/window.h"

#include "graphics/viewrenderer.h"
#include "graphics/scene.h"
#include "graphics/view.h"
#include "graphics/material.h"
#include "graphics/model.h"
#include "graphics/modelobject.h"

#include "utils/meshutils.h"

namespace vkr
{

	Application::Application()
		: m_WindowSize{}
	{
	}

	Application::~Application()
	{
	}

	ReturnCode Application::Launch(const ApplicationInitDesc& desc)
	{
		ReturnCode result = Init(desc);
		if (result != RETURN_OK)
			return result;

		result = MainLoop();
		if (result != RETURN_OK)
			return result;

		return Exit();
	}

	ReturnCode Application::Init(const ApplicationInitDesc& desc)
	{
		CommandLine::Parse(__argc, __argv);

		m_Window = MakeRef<Render::Window>(desc.m_WindowTitle.c_str(), desc.m_Resolution, desc.m_ShowCmd);

		m_RenderDevice = MakeUnique<Render::Device>();
		if (!m_RenderDevice->Init())
			return RETURN_ERROR;

		m_SwapChain = m_RenderDevice->CreateSwapChain(m_Window->GetNativeHandle(), desc.m_Resolution);
		if (!m_SwapChain)
			return RETURN_INVALID_ARG;

		//////////////////////////////////////////////////
		// these parts should not be in application
		m_ViewRenderer = MakeUnique<Graphics::ViewRenderer>();
		if (!m_ViewRenderer->Init())
			return RETURN_ERROR;

		m_Scene = MakeUnique<Graphics::Scene>();
		m_View = m_Scene->CreateView();
		m_View->SetRenderSize(desc.m_Resolution);
		//////////////////////////////////////////////////

		m_WindowSize = desc.m_Resolution;
		return RETURN_OK;
	}

	ReturnCode Application::MainLoop()
	{
		//////////////////////////////////////////////////
		// these parts should not be in application
		Ref<Graphics::Camera> camera = MakeRef<Graphics::Camera>();
		Mat43 camtransform = Compose(Mat33::Identity(), Vector3f(0, 0.0f, -2.0f));
		camera->SetLocalTransform(camtransform);
		camera->SetupPerspective(std::numbers::pi / 2.0f, (float)m_WindowSize.x / (float)m_WindowSize.y, 0.1f, 1000.0f);

		Ref<Render::Shader> VS = m_RenderDevice->CreateShader("../../../content/shaders/simpleforwardtestVS.hlsl", L"MainVS", vkr::Render::SHADER_STAGE_VERTEX, vkr::Render::ShaderModel::SM_6_0);
		Ref<Render::Shader> PS = m_RenderDevice->CreateShader("../../../content/shaders/simpleforwardtestPS.hlsl", L"MainPS", vkr::Render::SHADER_STAGE_PIXEL, vkr::Render::ShaderModel::SM_6_0);

		Render::PipelineStateDesc defpsodesc;
		defpsodesc.m_Type = vkr::Render::PIPELINE_STATE_TYPE_DEFAULT;
		defpsodesc.Default.m_PrimitiveType = vkr::Render::PRIMITIVE_TYPE_TRIANGLE;
		defpsodesc.Default.m_VertexLayout.m_Attributes.insert({ vkr::Render::VertexAttribute::TYPE_POSITION, 0, 0, vkr::Render::FORMAT_RGB32_FLOAT });
		defpsodesc.Default.m_VertexShader = VS.get();
		defpsodesc.Default.m_PixelShader = PS.get();
		defpsodesc.Default.m_RasterizerState = { vkr::Render::FACE_CULL_MODE_BACK, false, false, false };
		defpsodesc.Default.m_RenderTargetState = { {vkr::Render::Format::FORMAT_RGB10A2_UNORM} };
		defpsodesc.Default.m_DepthStencilState = { true, false, vkr::Render::COMPARISON_FUNC_EQUAL, Render::Format::FORMAT_D32_FLOAT };
		defpsodesc.Default.m_BlendState.RTBlends.push_back({true, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_SRC_ALPHA, vkr::Render::BLEND_INV_SRC_ALPHA, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_ONE, vkr::Render::BLEND_ZERO, vkr::Render::COLOR_WRITE_ALL});

		Ref<vkr::Render::PipelineState> cubemainpso = m_RenderDevice->CreatePipelineState(defpsodesc);

		Render::PipelineStateDesc depthpsodesc;
		depthpsodesc.m_Type = vkr::Render::PIPELINE_STATE_TYPE_DEFAULT;
		depthpsodesc.Default.m_PrimitiveType = vkr::Render::PRIMITIVE_TYPE_TRIANGLE;
		depthpsodesc.Default.m_VertexLayout.m_Attributes.insert({ vkr::Render::VertexAttribute::TYPE_POSITION, 0, 0, vkr::Render::FORMAT_RGB32_FLOAT });
		depthpsodesc.Default.m_VertexShader = VS.get();
		depthpsodesc.Default.m_PixelShader = nullptr;
		depthpsodesc.Default.m_RasterizerState = { vkr::Render::FACE_CULL_MODE_BACK, false, false, false };
		depthpsodesc.Default.m_RenderTargetState = { {vkr::Render::Format::FORMAT_RGB10A2_UNORM} };
		depthpsodesc.Default.m_DepthStencilState = { true, true, vkr::Render::COMPARISON_FUNC_GREATER_EQUAL, Render::Format::FORMAT_D32_FLOAT };
		depthpsodesc.Default.m_BlendState.RTBlends.push_back({ true, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_SRC_ALPHA, vkr::Render::BLEND_INV_SRC_ALPHA, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_ONE, vkr::Render::BLEND_ZERO, vkr::Render::COLOR_WRITE_ALL });

		Ref<vkr::Render::PipelineState> cubedepthpso = m_RenderDevice->CreatePipelineState(depthpsodesc);

		Ref<Graphics::Material> cubematerial = MakeRef<Graphics::Material>();
		cubematerial->SetDefaultPipelineState(cubemainpso);
		cubematerial->SetDepthPipelineState(cubedepthpso);

		Graphics::Model::Part part;
		part.m_Material = cubematerial;
		part.m_Mesh = vkr::CreateCubeMesh();
		Ref<Graphics::Model> cube = MakeRef<Graphics::Model>();
		cube->AddPart(part);

		Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>();
		modelinst->SetModel(cube);
		m_Scene->AddObject(modelinst);
		//////////////////////////////////////////////////

		bool running = true;
		while (running)
		{
			if (!m_Window->PeekMessages())
			{
				running = false;
			}

			// TODO: Apply changes coming from window messages
			// TODO: Apply changes going to window

			m_ElapsedTimer.Tick();

			m_RenderDevice->BeginFrame();

			//////////////////////////////////////////////////
			// these parts should not be in application
			modelinst->SetLocalTransform(Compose(CreateRotationZ(m_ElapsedTimer.ElapsedTime()), Vector3f(0, std::sin(m_ElapsedTimer.ElapsedTime()), 0)));

			m_View->SetOutputTarget(m_SwapChain->GetOutputRenderTarget());
			m_View->SetCamera(*camera);

			m_Scene->Update();
			m_Scene->PrepareView(*m_View);
			m_ViewRenderer->RenderView(*m_View);
			//////////////////////////////////////////////////

			m_SwapChain->Present();

			m_RenderDevice->EndFrame();
		}

		return RETURN_OK;
	}

	ReturnCode Application::Exit()
	{
		return RETURN_OK;
	}

}