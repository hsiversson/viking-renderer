#pragma once
#include "core/types.h"
#include "core/timer.h"

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

	struct ApplicationInitDesc
	{
		Vector2u m_Resolution;
		std::string m_WindowTitle;
		int32_t m_ShowCmd;
	};

	class Application
	{
	public:
		Application();
		~Application();

		ReturnCode Launch(const ApplicationInitDesc& desc);

	private:
		ReturnCode Init(const ApplicationInitDesc& desc);
		ReturnCode MainLoop();
		ReturnCode Exit();

	private:
		ElapsedTimer m_ElapsedTimer;
		UniquePtr<Render::Device> m_RenderDevice;
		Ref<Render::SwapChain> m_SwapChain;
		Ref<Render::Window> m_Window;

		// app probably shouldn't own these,
		// eventual GameWorld or some graphics module should.
		UniquePtr<Graphics::ViewRenderer> m_ViewRenderer;
		UniquePtr<Graphics::Scene> m_Scene;
		Ref<Graphics::View> m_View;

		Vector2u m_WindowSize;
	};
}