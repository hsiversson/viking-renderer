#pragma once
#include "core/types.h"

namespace vkr::Render
{
	class SwapChain;
}

namespace vkr
{
	struct IMessageHandler
	{
		virtual void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	};

	struct CreateWindowDesc
	{
		const char* m_WindowName = "Unnamed Window";
		Vector2u m_Position = { 100, 100};
		Vector2u m_Size = { 1280, 720 };
		int32_t m_ShowCmd = 1;

		void* m_ParentWindowHandle = nullptr;

		bool m_UseNativeTitlebar = false;
		bool m_IsDecorated = false;
		bool m_IsResizable = true;
		bool m_IsMaximized = false;
	};

	enum WindowChangeFlags
	{
		WINDOW_CHANGE_FLAG_NONE = 0,
		WINDOW_CHANGE_FLAG_SIZE = (1 << 0),
		WINDOW_CHANGE_FLAG_POSITION = (1 << 1),
	};

	class Window
	{
		friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	public:
		Window();
		~Window();

		bool Init(const CreateWindowDesc& createDesc);

		LRESULT ProcessMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

		bool PeekMessages();
		void AddMessageHandler(IMessageHandler* messageHandler) { m_MessageHandlers.insert(messageHandler); }

		void Show();
		void Hide();

		void Maximize(bool aValue);
		void Minimize();
		void Restore();

		void RequestAttention() const;
		void Focus() const;

		void SetAssociatedSwapChain(Render::SwapChain* swapChain);
		Render::SwapChain* GetAssociatedSwapChain() const;

		const Vector2u GetPosition() const;
		const Vector2u GetSize() const;
		uint32_t GetChangeFlags() const;
		void ResetChangeFlags();

		const Vector2f GetDpiScale() const;
		void* GetNativeHandle() const;


		static bool RegisterWindowClass(void* appIcon);
		static void UnregisterWindowClass();

	private:
		void HandlePositionChanged();

		void* m_NativeHandle;
		std::unordered_set<IMessageHandler*> m_MessageHandlers;
		Render::SwapChain* m_AssociatedSwapChain;

		Vector2u m_Position;
		Vector2u m_Size;
		uint32_t m_ChangeFlags;
	};
}