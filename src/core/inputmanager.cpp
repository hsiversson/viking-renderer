#include "inputmanager.h"

namespace vkr
{
	void InputManager::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_INPUT: //Mouse input processing using raw input
		{
			UINT dwSize = 0;
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, nullptr, &dwSize, sizeof(RAWINPUTHEADER));

			std::vector<BYTE> lpb(dwSize);
			if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
				break;

			RAWINPUT* raw = (RAWINPUT*)lpb.data();

			if (raw->header.dwType == RIM_TYPEMOUSE)
			{
				const RAWMOUSE& mouse = raw->data.mouse;

				if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) { ChangeInputState(MouseLeft, true); }
				if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) { ChangeInputState(MouseLeft, false); }
				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) { ChangeInputState(MouseRight, true); }
				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) { ChangeInputState(MouseRight, false); }
				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) { ChangeInputState(MouseRight, true); }
				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) { ChangeInputState(MouseRight, false); }
				if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) { ChangeInputState(MouseMiddle, true); }
				if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) { ChangeInputState(MouseMiddle, false); }
				if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) { ChangeInputState(MouseX1, true); }
				if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) { ChangeInputState(MouseX1, false); }
				if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) { ChangeInputState(MouseX2, true); }
				if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) { ChangeInputState(MouseX2, false); }
				if (mouse.usButtonFlags & RI_MOUSE_WHEEL) 
				{
					// Positive = scroll up, negative = scroll down
					int16_t wheelDelta = static_cast<int16_t>(mouse.usButtonData);
					m_CurrentState.m_WheelDelta += static_cast<float>(wheelDelta / INT16_MAX);
				}

				// For gaming mice with more buttons:
				if (mouse.ulRawButtons != 0)
				{
					// Bitmask of all buttons pressed — beyond 5
					// This is device-specific and depends on HID report format
				}

				// Raw movement (not screen coords):
				LONG dx = mouse.lLastX;
				LONG dy = mouse.lLastY;
				m_CurrentState.m_MouseDelta.x += dx;
				m_CurrentState.m_MouseDelta.y += dy;
			}
		}
		break;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			UINT vk = (UINT)wParam;
			switch (vk)
			{
			case 'W':
				ChangeInputState(KeyW, true);
				break;
			case 'A':
				ChangeInputState(KeyA, true);
				break;
			case 'S':
				ChangeInputState(KeyS, true);
				break;
			case 'D':
				ChangeInputState(KeyD, true);
				break;
			case 'Q':
				ChangeInputState(KeyQ, true);
				break;
			case 'E':
				ChangeInputState(KeyE, true);
				break;
			}
		}
		break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			UINT vk = (UINT)wParam;
			switch (vk)
			{
			case 'W':
				ChangeInputState(KeyW, false);
				break;
			case 'A':
				ChangeInputState(KeyA, false);
				break;
			case 'S':
				ChangeInputState(KeyS, false);
				break;
			case 'D':
				ChangeInputState(KeyD, false);
				break;
			case 'Q':
				ChangeInputState(KeyQ, false);
				break;
			case 'E':
				ChangeInputState(KeyE, false);
				break;
			}
		}
		break;
		case WM_MOUSEMOVE:
		{
			uint32_t x = static_cast<uint32_t>(LOWORD(lParam));
			uint32_t y = static_cast<uint32_t>(HIWORD(lParam));
			m_CurrentState.m_MousePosition.x = x;
			m_CurrentState.m_MousePosition.y = y;
		}
		break;
		}
	}

	void InputManager::EndFrame()
	{
		//Reset deltas
		m_CurrentState.m_MouseDelta = Vector2i(0);
		m_CurrentState.m_WheelDelta = 0;
	}

	bool InputManager::IsPressed(InputFlag Flag)
	{
		return m_CurrentState.m_InputFlags.test(Flag);
	}

	void InputManager::ChangeInputState(InputFlag Flag, bool Pressed)
	{
		if (Pressed)
			m_CurrentState.m_InputFlags.set(Flag);
		else
			m_CurrentState.m_InputFlags.reset(Flag);
	}

}