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
		Mat43 camtransform(Mat33::Identity, Vector3f(0, 0.0f, -2.0f));
		camera->SetLocalTransform(camtransform);
		camera->SetupPerspective(std::numbers::pi / 2.0f, (float)m_WindowSize.x / (float)m_WindowSize.y, 0.1f, 1000.0f);

		Ref<Render::Shader> VS = m_RenderDevice->CreateShader("../../../content/shaders/simpleforwardtestVS.hlsl", L"MainVS", vkr::Render::SHADER_STAGE_VERTEX, vkr::Render::ShaderModel::SM_6_0);
		Ref<Render::Shader> PS = m_RenderDevice->CreateShader("../../../content/shaders/simpleforwardtestPS.hlsl", L"MainPS", vkr::Render::SHADER_STAGE_PIXEL, vkr::Render::ShaderModel::SM_6_0);

		Render::PipelineStateDesc psodesc;
		psodesc.m_Type = vkr::Render::PIPELINE_STATE_TYPE_DEFAULT;
		psodesc.Default.m_PrimitiveType = vkr::Render::PRIMITIVE_TYPE_TRIANGLE;
		psodesc.Default.m_VertexLayout.m_Attributes.insert({ vkr::Render::VertexAttribute::TYPE_POSITION, 0, 0, vkr::Render::FORMAT_RGB32_FLOAT });
		psodesc.Default.m_VertexShader = VS.get();
		psodesc.Default.m_PixelShader = PS.get();
		psodesc.Default.m_RasterizerState = { vkr::Render::FACE_CULL_MODE_BACK, false, false, false };
		psodesc.Default.m_RenderTargetState = { {vkr::Render::Format::FORMAT_RGB10A2_UNORM} };
		psodesc.Default.m_DepthStencilState = { true, true, vkr::Render::COMPARISON_FUNC_GREATER, Render::Format::FORMAT_D32_FLOAT };
		psodesc.Default.m_BlendState.RTBlends.push_back({true, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_SRC_ALPHA, vkr::Render::BLEND_INV_SRC_ALPHA, vkr::Render::BLEND_OP_ADD, vkr::Render::BLEND_ONE, vkr::Render::BLEND_ZERO, vkr::Render::COLOR_WRITE_ALL});

		Ref<vkr::Render::PipelineState> cubemainpso = m_RenderDevice->CreatePipelineState(psodesc);
		Ref<Graphics::Material> cubematerial = MakeRef<Graphics::Material>();
		cubematerial->SetPipelineState(cubemainpso);

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

			m_ElapsedTimer.Tick();

			//////////////////////////////////////////////////
			// these parts should not be in application
			modelinst->SetLocalTransform(vkr::Mat43(vkr::Mat33::CreateRotationZ(m_ElapsedTimer.ElapsedTime()), Vector3f(0, std::sin(m_ElapsedTimer.ElapsedTime()), 0)));

			Ref<vkr::Render::Texture> backbuffer = m_SwapChain->GetOutputTexture();
			Render::ResourceDescriptorDesc rtdesc;
			rtdesc.Type = vkr::Render::ResourceDescriptorType::RTV;
			Ref<Render::RenderTargetView> rtdescriptor = m_RenderDevice->CreateRTView(backbuffer.get(), rtdesc);
			m_View->SetOutputTarget(rtdescriptor);
			m_View->SetCamera(*camera);

			m_Scene->Update();
			m_Scene->PrepareView(*m_View);
			m_ViewRenderer->RenderView(*m_View);
			//////////////////////////////////////////////////

			m_SwapChain->Present();

			m_RenderDevice->GarbageCollect();
		}

		return RETURN_OK;
	}

	ReturnCode Application::Exit()
	{
		return RETURN_OK;
	}

}