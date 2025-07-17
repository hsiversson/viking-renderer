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

		m_InputManager = MakeUnique<InputManager>();

		m_Window->AddMessageHandler(m_InputManager.get());

		m_RenderDevice = MakeUnique<Render::Device>();
		if (!m_RenderDevice->Init())
			return RETURN_ERROR;

		m_SwapChain = m_RenderDevice->CreateSwapChain(m_Window->GetNativeHandle(), desc.m_Resolution);
		if (!m_SwapChain)
			return RETURN_INVALID_ARG;

		m_Scene = MakeUnique<Graphics::Scene>();

		m_ViewRenderer = MakeUnique<Graphics::ViewRenderer>();
		if (!m_ViewRenderer->Init())
			return RETURN_ERROR;

		
		m_View = m_Scene->CreateView();
		m_View->SetRenderSize(desc.m_Resolution);

		m_WindowSize = desc.m_Resolution;

		

		AppInit();
		return RETURN_OK;
	}

	ReturnCode Application::MainLoop()
	{
		MovingAverage<uint32_t, 64> m_FpsMovingAverage;
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
			m_FpsMovingAverage.Add(static_cast<uint32_t>(std::roundf(1.0f / m_ElapsedTimer.DeltaTime())));
			//VKR_LOG("FPS: {}", m_FpsMovingAverage.GetAverage());

			// App tick
			Tick(m_ElapsedTimer.DeltaTime());

			// TODO: Move scene/world ownership into app? 
			m_Scene->Update();
			m_Scene->PrepareView(*m_View);

			{
				Render::QueueGraphicsTask(std::bind(&Render::Device::BeginFrame, m_RenderDevice.get()));
				Render::QueueGraphicsTask([this]() { m_View->SetOutputTarget(m_SwapChain->GetOutputRenderTarget()); });
				m_ViewRenderer->RenderView(*m_View);
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