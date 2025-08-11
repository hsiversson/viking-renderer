#pragma once

#include "core/inputmanager.h"
#include "core/timer.h"
#include "core/types.h"

namespace vkr
{
	namespace Render
	{
		class Device;
		class NvStreamline;
		class SwapChain;
	}

	namespace Editor
	{
		class Manager;
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
		std::filesystem::path m_ExePath;
		std::filesystem::path m_ContentDirectory;
		int32_t m_ShowCmd;
		ApplicationMode m_Mode;
	};

	class Window;
	class Application
	{
	public:
		Application();
		virtual ~Application();

		ReturnCode Launch(const ApplicationInitDesc& desc);

		void SetCurrentSwapChain(const Ref<Render::SwapChain>& swapChain);

		virtual void AppInit() {}
		virtual void Tick(float deltaTime) {}
		virtual void AppShutdown() {}

		Window* GetMainWindow() const;

		static Application* Get();
		static void RequestQuit(ReturnCode returnCode = RETURN_OK);
	private:
		ReturnCode Init(const ApplicationInitDesc& desc);
		ReturnCode MainLoop();
		ReturnCode Exit();

	protected:
		ElapsedTimer m_ElapsedTimer;

		Ref<Render::SwapChain> m_SwapChain;
		UniquePtr<Render::Device> m_RenderDevice;
		UniquePtr<Render::NvStreamline> m_NvStreamline;
		Ref<Window> m_Window;

//#if !BUILD_CONFIG_SHIPPING
		UniquePtr<Editor::Manager> m_EditorManager;
//#endif
		Vector2u m_WindowSize;

		UniquePtr<InputManager> m_InputManager;

		bool m_QuitRequested = false;
		ReturnCode m_QuitReturnCode = RETURN_OK;

		bool m_UseDLSS = true;

		static Application* g_Instance;
	};
}