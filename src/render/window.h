#pragma once
#include "core/types.h"

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

		const Vector2u GetSize() const;
		const Vector2f GetDpiScale() const;

	private:
		void* m_NativeHandle;
		std::unordered_set<IMessageHandler*> m_MessageHandlers;
	};
}