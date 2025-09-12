#include "application.h"
#include "core/commandline.h"
#include "core/logger.h"

#include "render/device.h"
#include "render/nvstreamline.h"
#include "window.h"

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
	Application* Application::g_Instance = nullptr;

	Application::Application()
		: m_WindowSize{}
	{
		g_Instance = this;
	}

	Application::~Application()
	{
		m_EditorManager.reset();
		m_Window.reset();

		Logger::Destroy();
		ThreadPool::Destroy();
		Window::UnregisterWindowClass();

		g_Instance = nullptr;
	}

	ReturnCode Application::Launch(const ApplicationInitDesc& desc)
	{
		Thread::RegisterMainThread();
		SystemPaths::Init(desc.m_ExePath, desc.m_ContentDirectory);
		ThreadPool::Create();
		Logger::Create();

		ReturnCode result = Init(desc);
		if (result != RETURN_OK)
			return result;

		result = MainLoop();
		if (result != RETURN_OK)
			return result;

		return Exit();
	}

	void Application::SetCurrentSwapChain(const Ref<Render::SwapChain>& swapChain)
	{
		VKR_ASSERT(Thread::IsMainThread());
		Render::GetDevice()->SetCurrentSwapChain(swapChain);
		m_Window->SetAssociatedSwapChain(swapChain.get());
	}

	Window* Application::GetMainWindow() const
	{
		return m_Window.get();
	}

	Application* Application::Get()
	{
		return g_Instance;
	}

	void Application::RequestQuit(ReturnCode returnCode)
	{
		if (Application* app = Get())
		{
			app->m_QuitRequested = true;
			app->m_QuitReturnCode = returnCode;
		}
	}

	ReturnCode Application::Init(const ApplicationInitDesc& desc)
	{
		CommandLine::Parse(__argc, __argv);

		m_AppSettings = MakeUnique<AppSettings>();

		Window::RegisterWindowClass(nullptr);

		CreateWindowDesc windowDesc = {};
		windowDesc.m_Size = desc.m_Resolution;
		windowDesc.m_Position = Vector2u(100, 100);
		windowDesc.m_ShowCmd = desc.m_ShowCmd;
		windowDesc.m_WindowName = desc.m_WindowTitle.c_str();
		windowDesc.m_IsResizable = true;
		windowDesc.m_IsDecorated = false;
		windowDesc.m_IsMaximized = false;

		m_Window = MakeRef<Window>();
		if (!m_Window->Init(windowDesc))
		{
			return RETURN_ERROR;
		}
		m_Window->Hide();

		m_InputManager = MakeUnique<InputManager>();

		m_Window->AddMessageHandler(m_InputManager.get());

		m_RenderDevice = MakeUnique<Render::Device>();
		if (!m_RenderDevice->Init())
		{
			return RETURN_ERROR;
		}

		m_SwapChain = m_RenderDevice->CreateSwapChain(m_Window->GetNativeHandle(), desc.m_Resolution);
		if (!m_SwapChain)
			return RETURN_INVALID_ARG;
		SetCurrentSwapChain(m_SwapChain);

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
		bool isFirstIteration = true;
		while (true)
		{
			if (m_QuitRequested)
			{
				break;
			}

			if (!m_Window->PeekMessages())
			{
				break;
			}

			const uint32_t windowChangeFlags = m_Window->GetChangeFlags();
			if (windowChangeFlags > WINDOW_CHANGE_FLAG_NONE)
			{
				m_SwapChain->Resize(m_Window->GetSize());
				m_Window->ResetChangeFlags();
			}
			// TODO: Apply changes going to window

			m_ElapsedTimer.Tick();
			//VKR_LOG("FPS: {}", m_FpsMovingAverage.GetAverage());

			Render::GetDevice()->BeginFrame(m_ElapsedTimer.FrameIndex());

			if (m_EditorManager)
				m_EditorManager->Update();

			// App tick
			Tick(m_ElapsedTimer.DeltaTime());

			if (m_EditorManager)
				m_EditorManager->Render();

			Render::QueueGraphicsTask(std::bind(&Render::SwapChain::Present, m_SwapChain.get()));
			Render::GetDevice()->EndFrame();
			m_InputManager->EndFrame();

			ThreadPool::Get().WaitForShortTasks();

			// TODO: we shouldn't need this, but is done because we don't want to show an empty window for one or two frames.
			if (isFirstIteration)
			{
				m_Window->Show();
				isFirstIteration = false;
			}
		}

		m_RenderDevice->WaitForGpuIdle();
		
		return m_QuitReturnCode;
	}

	ReturnCode Application::Exit()
	{
		return RETURN_OK;
	}

}