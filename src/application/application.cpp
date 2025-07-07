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

#include "graphics/modelloader_gltf.h"

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

		m_RenderDevice->BeginFrame();

		Graphics::ModelLoader_GLTF loader;
		Ref<Graphics::Model> model;
		model = loader.Load("../../../content/models/cp_noodles/scene.gltf");
		Ref<Graphics::ModelObject> modelinst = MakeRef<Graphics::ModelObject>(); 
		modelinst->SetLocalTransform(Compose(Mat33::Identity(), Vector3f(0.0f, 0.0f, 0.0f)));

		modelinst->SetModel(model);
		m_Scene->AddObject(modelinst);

		Ref<Graphics::Camera> camera = MakeRef<Graphics::Camera>();
		Mat43 camtransform = Compose(Mat33::Identity(), Vector3f(0, 2.0f, -4.0f));
		camera->SetLocalTransform(camtransform);
		camera->SetupPerspective(std::numbers::pi / 2.0f, (float)m_WindowSize.x / (float)m_WindowSize.y, 0.1f, 1000.0f);

		m_RenderDevice->EndFrame();
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
			//modelinst->SetLocalTransform(Compose(CreateRotationY(m_ElapsedTimer.ElapsedTime() * 0.25f), Vector3f(0.0f, 0.0f, 0.0f)));

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