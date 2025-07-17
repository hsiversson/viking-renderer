#pragma once

#include "core/inputmanager.h"
#include "core/timer.h"
#include "core/types.h"

namespace vkr
{
	namespace Render
	{
		class Device;
		class SwapChain;
		class Window;
	}

	namespace Graphics
	{
		class ViewRenderer;
		class View;
		class Scene;
	}

	enum class ApplicationMode
	{
		Runtime,
		Editor
	};

	struct ApplicationInitDesc
	{
		Vector2u m_Resolution;
		std::string m_WindowTitle;
		int32_t m_ShowCmd;
		ApplicationMode m_Mode;
	};

	class Application
	{
	public:
		Application();
		~Application();

		ReturnCode Launch(const ApplicationInitDesc& desc);

		virtual void AppInit() {}
		virtual void Tick(float deltaTime) {}
		virtual void AppShutdown() {}

	private:
		ReturnCode Init(const ApplicationInitDesc& desc);
		ReturnCode MainLoop();
		ReturnCode Exit();

	protected:
		ElapsedTimer m_ElapsedTimer;

		// app probably shouldn't own these,
		// eventual GameWorld or some graphics module should.
		UniquePtr<Graphics::ViewRenderer> m_ViewRenderer;
		Ref<Graphics::View> m_View;
		UniquePtr<Graphics::Scene> m_Scene;

		Ref<Render::SwapChain> m_SwapChain;
		UniquePtr<Render::Device> m_RenderDevice;
		Ref<Render::Window> m_Window;

		Vector2u m_WindowSize;

		UniquePtr<InputManager> m_InputManager;
	};
}