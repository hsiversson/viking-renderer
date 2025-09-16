#include "window.h"

#include <windowsx.h>
#include "core/resource/resource.h"

#ifdef IsMaximized
#undef IsMaximized
#endif

namespace vkr
{
	static constexpr const char* g_WindowClassName = "VKR_WND_CLASS";

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
		, m_BorderThickness(1)
		, m_ChangeFlags(WINDOW_CHANGE_FLAG_NONE)
		, m_IsMaximized(false)
	{
	}

	Window::~Window()
	{
		DestroyWindow((HWND)m_NativeHandle);
	}

	bool Window::Init(const CreateWindowDesc& createDesc)
	{
		m_BorderThickness = createDesc.m_BorderThickness;
		m_IsMaximized = createDesc.m_IsMaximized;
		m_Name = createDesc.m_WindowName;

		const uint32_t windowStyle = ConvertToWindowStyle(createDesc, createDesc.m_IsMaximized);

		RECT rc = { 0, 0, (LONG)createDesc.m_Size.x, (LONG)createDesc.m_Size.y };
		AdjustWindowRect(&rc, windowStyle, false);
		Vector2u actualSize = { rc.right - rc.left, rc.bottom - rc.top };

		m_NativeHandle = CreateWindowEx(
			WS_EX_APPWINDOW,
			g_WindowClassName, 
			createDesc.m_WindowName, 
			windowStyle,
			createDesc.m_Position.x, createDesc.m_Position.y,
			actualSize.x, actualSize.y, 
			nullptr, 
			nullptr, 
			nullptr, 
			this);

		::SetWindowLongPtr((HWND)m_NativeHandle, GWLP_USERDATA, LONG_PTR(this));
		::SetWindowLongPtr((HWND)m_NativeHandle, GWL_STYLE, windowStyle);

		RAWINPUTDEVICE rid = {};
		rid.usUsagePage = 0x01; // Generic desktop controls
		rid.usUsage = 0x02;     // Mouse
		rid.dwFlags = RIDEV_INPUTSINK; // Or RIDEV_NOLEGACY to suppress WM_MOUSE*
		rid.hwndTarget = (HWND)m_NativeHandle;

		RegisterRawInputDevices(&rid, 1, sizeof(rid));

		UpdateWindow((HWND)m_NativeHandle);
		ShowWindow((HWND)m_NativeHandle, SW_HIDE);
		Focus();
		Maximize(m_IsMaximized);
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
		case WM_WINDOWPOSCHANGED:
			HandlePositionChanged();
			return 0;
		case WM_NCCALCSIZE:
			if (lParam)
				return 0;
			break;
		case WM_NCHITTEST:
		{
			POINT cursorPos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			ScreenToClient(hwnd, &cursorPos);

			if (!m_IsMaximized)
			{
				RECT rc;
				GetClientRect(hwnd, &rc);

				const int32_t verticalBorderSize = GetSystemMetrics(SM_CYFRAME);

				enum : uint8_t
				{
					Left = 0x1,
					Top = 0x2,
					Right = 0x4,
					Bottom = 0x8
				};

				uint8_t hit = 0;
				if (cursorPos.x <= m_BorderThickness)
					hit |= Left;
				if (cursorPos.x >= rc.right - m_BorderThickness)
					hit |= Right;
				if (cursorPos.y <= m_BorderThickness || cursorPos.y < verticalBorderSize)
					hit |= Top;
				if (cursorPos.y >= rc.bottom - m_BorderThickness)
					hit |= Bottom;

				if (hit & Top && hit & Left)        
					return HTTOPLEFT;
				if (hit & Top && hit & Right)       
					return HTTOPRIGHT;
				if (hit & Bottom && hit & Left)     
					return HTBOTTOMLEFT;
				if (hit & Bottom && hit & Right)    
					return HTBOTTOMRIGHT;
				if (hit & Left)                     
					return HTLEFT;
				if (hit & Top)                      
					return HTTOP;
				if (hit & Right)                    
					return HTRIGHT;
				if (hit & Bottom)                   
					return HTBOTTOM;
			}

			if (m_IsTitlebarHoveredCallback && m_IsTitlebarHoveredCallback(cursorPos.x, cursorPos.y))
				return HTCAPTION;

			return HTCLIENT;
		}
		case WM_GETMINMAXINFO:
		{
			// lParam points to a MINMAXINFO struct
			MINMAXINFO* mmi = (MINMAXINFO*)lParam;

			// Get the work area (excluding taskbar)
			RECT rcWorkArea;
			SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWorkArea, 0);

			// Set the max position (top-left corner)
			mmi->ptMaxPosition.x = rcWorkArea.left;
			mmi->ptMaxPosition.y = rcWorkArea.top;

			// Set the max size (width/height)
			mmi->ptMaxSize.x = rcWorkArea.right - rcWorkArea.left;
			mmi->ptMaxSize.y = rcWorkArea.bottom - rcWorkArea.top;

			return 0;
		}
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
		ShowWindow((HWND)m_NativeHandle, SW_SHOW);
	}

	void Window::Hide()
	{
		ShowWindow((HWND)m_NativeHandle, SW_HIDE);
	}

	bool Window::IsMaximized() const
	{
		return m_IsMaximized;
	}

	void Window::Maximize(bool aValue)
	{
		ShowWindow((HWND)m_NativeHandle, (aValue) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL);
		m_IsMaximized = aValue;
	}

	void Window::Minimize()
	{
		ShowWindow((HWND)m_NativeHandle, SW_MINIMIZE);
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

	void Window::SetIsTitlebarHoveredCallback(IsTitleBarHoveredFn callback)
	{
		m_IsTitlebarHoveredCallback = callback;
	}

	const Vector2u Window::GetPosition() const
	{
		return m_Position;
	}

	const Vector2u Window::GetSize() const
	{
		return m_Size;
	}

	uint32_t Window::GetChangeFlags() const
	{
		return m_ChangeFlags;
	}

	void Window::ResetChangeFlags()
	{
		VKR_ASSERT(Thread::IsMainThread());
		m_ChangeFlags = WINDOW_CHANGE_FLAG_NONE;
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

	const std::string& Window::GetName() const
	{
		return m_Name;
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

		windowClass.hIcon = (HICON)LoadImage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APP_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);
		if (!windowClass.hIcon)
			windowClass.hIcon = (HICON)LoadImage(nullptr, IDI_APPLICATION, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE | LR_SHARED);

		if (!RegisterClassEx(&windowClass))
		{
			uint32_t errorCode = GetLastError();
			VKR_ASSERT(false, "Failed to register window class.");
			return false;
		}

		return true;
	}

	void Window::UnregisterWindowClass()
	{
		if (!UnregisterClass(g_WindowClassName, GetModuleHandle(nullptr)))
		{
			uint32_t errorCode = GetLastError();
			VKR_ASSERT(false, "Failed to unregister window class");
		}
	}

	void Window::HandlePositionChanged()
	{
		HWND nativeHandle = (HWND)m_NativeHandle;
		if (::IsIconic(nativeHandle) /*&& IsMinimized()*/)
			Minimize();

		WINDOWPLACEMENT placement = {};
		placement.length = sizeof(WINDOWPLACEMENT);
		::GetWindowPlacement(nativeHandle, &placement);

		if ((placement.showCmd == SW_SHOWNORMAL) || (placement.showCmd == SW_SHOWMAXIMIZED))
		{
			RECT borders = {};
			DWORD windowStyle = static_cast<DWORD>(::GetWindowLong(nativeHandle, GWL_STYLE));
			::AdjustWindowRect(&borders, windowStyle, false);

			RECT rect = {};
			::GetWindowRect(nativeHandle, &rect);
			rect.left -= borders.left;
			rect.right -= borders.right;
			rect.top -= borders.top;
			rect.bottom -= borders.bottom;

			m_Position = Vector2u(rect.left, rect.top);
			m_ChangeFlags |= WINDOW_CHANGE_FLAG_POSITION;

			m_Size = Vector2u(rect.right - rect.left, rect.bottom - rect.top);
			m_ChangeFlags |= WINDOW_CHANGE_FLAG_SIZE;
		}
	}

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		Window* app = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (app)
		{
			return app->ProcessMessage(hwnd, msg, wParam, lParam);
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
}