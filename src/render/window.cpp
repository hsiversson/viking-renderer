#include "window.h"

namespace vkr::Render
{
	static constexpr const char* g_WindowClassName = "VKR_WND_CLASS";

	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static uint32_t ConvertToWindowStyle(const CreateWindowDesc& createDesc, bool isMaximized)
	{
		DWORD windowStyle = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
		windowStyle |= createDesc.m_IsDecorated ? ((createDesc.m_IsResizable ? WS_OVERLAPPEDWINDOW : (WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX))) : 0;

		if (isMaximized)
		{
			windowStyle |= WS_MAXIMIZE;
		}

		return windowStyle;
	}

	Window::Window()
		: m_NativeHandle(nullptr)
		, m_AssociatedSwapChain(nullptr)
	{
	}

	Window::~Window()
	{
	}

	bool Window::Init(const CreateWindowDesc& createDesc)
	{
		const uint32_t windowStyle = ConvertToWindowStyle(createDesc, createDesc.m_IsMaximized);

		RECT rc = { 0, 0, (LONG)createDesc.m_Size.x, (LONG)createDesc.m_Size.y };
		AdjustWindowRect(&rc, windowStyle, false);
		Vector2u actualSize = { rc.right - rc.left, rc.bottom - rc.top };

		m_NativeHandle = CreateWindow(
			g_WindowClassName, 
			createDesc.m_WindowName, 
			windowStyle,
			createDesc.m_Position.x, createDesc.m_Position.y,
			actualSize.x, actualSize.y, 
			nullptr, 
			nullptr, 
			nullptr, 
			this);

		RAWINPUTDEVICE rid = {};
		rid.usUsagePage = 0x01; // Generic desktop controls
		rid.usUsage = 0x02;     // Mouse
		rid.dwFlags = RIDEV_INPUTSINK; // Or RIDEV_NOLEGACY to suppress WM_MOUSE*
		rid.hwndTarget = (HWND)m_NativeHandle;

		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		//ShowCursor(false);

		ShowWindow((HWND)m_NativeHandle, createDesc.m_ShowCmd);
		UpdateWindow((HWND)m_NativeHandle);
		Focus();
		return true;
	}

	LRESULT Window::ProcessMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		//If a handler handles the message should we do anything else with it?
		for (auto handler : m_MessageHandlers)
		{
			handler->ProcessMessage(Msg, wParam, lParam);
		}

		switch (Msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, Msg, wParam, lParam);
	}

	bool Window::PeekMessages()
	{
		MSG msg = {};
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return true;
	}

	void Window::Show()
	{
		ShowWindow((HWND)m_NativeHandle, SW_SHOWNA);
	}

	void Window::Hide()
	{
		ShowWindow((HWND)m_NativeHandle, SW_HIDE);
	}

	void Window::Maximize(bool aValue)
	{
		SendMessage((HWND)m_NativeHandle, WM_SYSCOMMAND, (aValue) ? SC_MAXIMIZE : SC_RESTORE, 0);
	}

	void Window::Minimize()
	{
	}

	void Window::Restore()
	{
		ShowWindow((HWND)m_NativeHandle, SW_RESTORE);
	}

	void Window::RequestAttention() const
	{
		FlashWindow((HWND)m_NativeHandle, true);
	}

	void Window::Focus() const
	{
		BringWindowToTop((HWND)m_NativeHandle);
		SetForegroundWindow((HWND)m_NativeHandle);
		SetFocus((HWND)m_NativeHandle);
	}

	void Window::SetAssociatedSwapChain(Render::SwapChain* swapChain)
	{
		m_AssociatedSwapChain = swapChain;
	}

	Render::SwapChain* Window::GetAssociatedSwapChain() const
	{
		return m_AssociatedSwapChain;
	}

	const Vector2u Window::GetSize() const
	{
		RECT clientRect;
		GetClientRect((HWND)m_NativeHandle, &clientRect);
		int width = clientRect.right - clientRect.left;
		int height = clientRect.bottom - clientRect.top;
		return Vector2u(width, height);
	}

	const Vector2f Window::GetDpiScale() const
	{
		const uint32_t dpi = GetDpiForWindow((HWND)m_NativeHandle);
		Vector2f dpiScale;
		dpiScale.x = dpi / (float)USER_DEFAULT_SCREEN_DPI;
		dpiScale.y = dpi / (float)USER_DEFAULT_SCREEN_DPI;
		return dpiScale;
	}

	void* Window::GetNativeHandle() const
	{
		return m_NativeHandle;
	}

	bool Window::RegisterWindowClass(void* appIcon)
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		WNDCLASSEX windowClass = {};
		windowClass.cbSize = sizeof(windowClass);
		windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		windowClass.lpfnWndProc = &WndProc;
		windowClass.hInstance = GetModuleHandle(nullptr);
		windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
		windowClass.lpszClassName = g_WindowClassName;

		windowClass.hIcon = (HICON)appIcon;
		if (!windowClass.hIcon)
			windowClass.hIcon = (HICON)LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

		if (!RegisterClassEx(&windowClass))
		{
			uint32_t errorCode = GetLastError();
			assert(false && "Failed to register window class.");
			return false;
		}

		return true;
	}

	void Window::UnregisterWindowClass()
	{
		if (!UnregisterClass(g_WindowClassName, GetModuleHandle(nullptr)))
		{
			uint32_t errorCode = GetLastError();
			assert(false && "Failed to unregister window class");
		}
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		if (Msg == WM_NCCREATE) 
		{
			// This is the first message received — set the user data
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			Window* pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
			return DefWindowProc(hwnd, Msg, wParam, lParam);
		}

		Window* app = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (app)
		{
			return app->ProcessMessage(hwnd, Msg, wParam, lParam);
		}

		return DefWindowProc(hwnd, Msg, wParam, lParam);
		
	}
}