#include "window.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

namespace vkr::Render
{
	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

	static constexpr const char* g_WindowClassName = "VKR_WND_CLASS";

	Window::Window(const char* name, const Vector2u& size, int32_t showCmd)
	{

		WNDCLASSEX WndClsEx = {};
		WndClsEx.cbSize = sizeof(WNDCLASSEX);
		WndClsEx.style = CS_HREDRAW | CS_VREDRAW;
		WndClsEx.lpfnWndProc = WndProc;
		WndClsEx.hInstance = nullptr;
		WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		WndClsEx.lpszClassName = g_WindowClassName;
		WndClsEx.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		RegisterClassEx(&WndClsEx);

		m_NativeHandle = CreateWindow(g_WindowClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, size.x, size.y, nullptr, nullptr, nullptr, nullptr);

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

	LRESULT CALLBACK WndProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
	{
		switch (Msg)
		{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hwnd, Msg, wParam, lParam);
		}
		return 0;
	}
}