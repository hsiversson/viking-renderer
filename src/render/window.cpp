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

	Window::Window(const char* name, const Vector2u& size, int32_t showCmd)
	{
		static constexpr const char* windowClassName = "VKR_WND_CLASS";

		WNDCLASSEX WndClsEx = {};
		WndClsEx.cbSize = sizeof(WNDCLASSEX);
		WndClsEx.style = CS_HREDRAW | CS_VREDRAW;
		WndClsEx.lpfnWndProc = WndProc;
		WndClsEx.hInstance = nullptr;
		WndClsEx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
		WndClsEx.lpszClassName = windowClassName;
		WndClsEx.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
		RegisterClassEx(&WndClsEx);

		m_NativeHandle = CreateWindow(windowClassName, name, WS_OVERLAPPEDWINDOW, 100, 100, size.x, size.y, nullptr, nullptr, nullptr, nullptr);

		ShowWindow((HWND)m_NativeHandle, showCmd);
		UpdateWindow((HWND)m_NativeHandle);
	}

	Window::~Window()
	{

	}

	void* Window::GetNativeHandle() const
	{
		return m_NativeHandle;
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