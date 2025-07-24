#include "window.h"



namespace vkr::Render
{
	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static constexpr const char* g_WindowClassName = "VKR_WND_CLASS";

	Window::Window(const char* name, const Vector2u& size, int32_t showCmd)
	{
		SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

		RECT rc = { 0, 0, (LONG)size.x, (LONG)size.y };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		Vector2u actualSize = { rc.right - rc.left, rc.bottom - rc.top };

		WNDCLASSEX WndClsEx = {};
		WndClsEx.cbSize = sizeof(WNDCLASSEX);
		WndClsEx.style = CS_HREDRAW | CS_VREDRAW;
		WndClsEx.lpfnWndProc = WndProc;
		WndClsEx.hInstance = nullptr;
		WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		WndClsEx.lpszClassName = g_WindowClassName;
		WndClsEx.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		RegisterClassEx(&WndClsEx);

		m_NativeHandle = CreateWindow(g_WindowClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, actualSize.x, actualSize.y, nullptr, nullptr, nullptr, this);

		RAWINPUTDEVICE rid = {};
		rid.usUsagePage = 0x01; // Generic desktop controls
		rid.usUsage = 0x02;     // Mouse
		rid.dwFlags = RIDEV_INPUTSINK; // Or RIDEV_NOLEGACY to suppress WM_MOUSE*
		rid.hwndTarget = (HWND)m_NativeHandle;

		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		//ShowCursor(false);

		ShowWindow((HWND)m_NativeHandle, showCmd);
		UpdateWindow((HWND)m_NativeHandle);
	}

	Window::~Window()
	{
		UnregisterClass(g_WindowClassName, nullptr);
	}

	void* Window::GetNativeHandle() const
	{
		return m_NativeHandle;
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
		dpiScale.x = dpi / 96.0f;
		dpiScale.y = dpi / 96.0f;
		return dpiScale;
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

	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		if (Msg == WM_NCCREATE) {
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