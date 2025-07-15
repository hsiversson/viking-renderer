#pragma once
#include "core/types.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace vkr::Render
{
	struct IMessageHandler
	{
		virtual void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	class Window
	{
	public:
		Window(const char* name, const Vector2u& size, int32_t showCmd);
		~Window();

		LRESULT ProcessMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

		bool PeekMessages();
		void* GetNativeHandle() const;
		void AddMessageHandler(IMessageHandler* messageHandler) { m_MessageHandlers.insert(messageHandler); }

	private:
		void* m_NativeHandle;
		std::unordered_set<IMessageHandler*> m_MessageHandlers;
	};
}