#include "application.h"
#include "core/commandline.h"
#include "core/logger.h"

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

// TEMP
#include "editor/editor.h"
// TEMP

namespace vkr
{

	Application::Application()
		: m_WindowSize{}
	{
		Logger::Create();
	}

	Application::~Application()
	{
		m_Scene->DestroyView(m_View);
		Logger::Destroy();
		Render::Window::UnregisterWindowClass();
	}

	ReturnCode Application::Launch(const ApplicationInitDesc& desc)
	{
		Thread::RegisterMainThread();

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

		Render::Window::RegisterWindowClass(nullptr);

		Render::CreateWindowDesc windowDesc = {};
		windowDesc.m_Size = desc.m_Resolution;
		windowDesc.m_Position = Vector2u(100, 100);
		windowDesc.m_ShowCmd = desc.m_ShowCmd;
		windowDesc.m_WindowName = desc.m_WindowTitle.c_str();
		windowDesc.m_IsResizable = true;
		windowDesc.m_IsDecorated = true;
		windowDesc.m_IsMaximized = false;

		m_Window = MakeRef<Render::Window>();
		if (!m_Window->Init(windowDesc))
		{
			return RETURN_ERROR;
		}

		m_InputManager = MakeUnique<InputManager>();

		m_Window->AddMessageHandler(m_InputManager.get());

		m_RenderDevice = MakeUnique<Render::Device>();
		if (!m_RenderDevice->Init())
			return RETURN_ERROR;

		m_SwapChain = m_RenderDevice->CreateSwapChain(m_Window->GetNativeHandle(), desc.m_Resolution);
		if (!m_SwapChain)
			return RETURN_INVALID_ARG;
		m_Window->SetAssociatedSwapChain(m_SwapChain.get());

		m_Scene = MakeUnique<Graphics::Scene>();

		m_View = m_Scene->CreateView();
		m_View->SetRenderSize(desc.m_Resolution);

		m_ViewRenderer = MakeUnique<Graphics::ViewRenderer>();
		if (!m_ViewRenderer->Init(*m_View))
			return RETURN_ERROR;

		m_WindowSize = desc.m_Resolution;

		if (desc.m_Mode == ApplicationMode::Editor)
		{
			m_EditorManager = MakeUnique<Editor::Manager>();
			if (!m_EditorManager->Init(m_InputManager.get(), m_Window))
				return RETURN_ERROR;
		}

		AppInit();
		return RETURN_OK;
	}

	ReturnCode Application::MainLoop()
	{
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
			//VKR_LOG("FPS: {}", m_FpsMovingAverage.GetAverage());

			if (m_EditorManager)
				m_EditorManager->Update();

			// App tick
			Tick(m_ElapsedTimer.DeltaTime());

			// TODO: Move scene/world ownership into app? 
			m_Scene->Update();
			m_Scene->PrepareView(*m_View);

			{
				Render::QueueGraphicsTask(std::bind(&Render::Device::BeginFrame, m_RenderDevice.get()));
				Render::QueueGraphicsTask([this]() 
					{ 
						m_View->SetOutputTarget(m_SwapChain->GetOutputRenderTarget()); 

						if (m_EditorManager)
							m_EditorManager->SetOutputTarget(m_SwapChain->GetOutputRenderTarget());
					});
				m_ViewRenderer->RenderView(*m_View);

				if (m_EditorManager)
					m_EditorManager->Draw();

				Render::QueueGraphicsTask(std::bind(&Render::SwapChain::Present, m_SwapChain.get()));
				Render::QueueGraphicsTask(std::bind(&Render::Device::EndFrame, m_RenderDevice.get()));
			}
			m_InputManager->EndFrame();
		}

		m_RenderDevice->WaitForGpuIdle();
		
		return RETURN_OK;
	}

	ReturnCode Application::Exit()
	{
		return RETURN_OK;
	}

}