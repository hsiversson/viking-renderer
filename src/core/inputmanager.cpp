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

				if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_LEFT] = true; }
				if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_LEFT] = false; }

				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_RIGHT] = true; }
				if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_RIGHT] = false; }

				if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_MIDDLE] = true; }
				if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) { m_CurrentMouseState.m_KeyStates[INPUT_MOUSE_KEY_MIDDLE] = false; }

				//if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) { ChangeInputState(MouseX1, true); }
				//if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) { ChangeInputState(MouseX1, false); }
				//if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) { ChangeInputState(MouseX2, true); }
				//if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) { ChangeInputState(MouseX2, false); }
				if (mouse.usButtonFlags & RI_MOUSE_WHEEL) 
				{
					// Positive = scroll up, negative = scroll down
					int16_t wheelDelta = static_cast<int16_t>(mouse.usButtonData);
					m_CurrentMouseState.m_WheelDelta += static_cast<float>(wheelDelta / INT16_MAX);
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
				m_CurrentMouseState.m_MouseDelta.x += dx;
				m_CurrentMouseState.m_MouseDelta.y += dy;
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
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_W] = true;
				break;
			case 'A':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_A] = true;
				break;
			case 'S':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_S] = true;
				break;
			case 'D':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_D] = true;
				break;
			case 'Q':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_Q] = true;
				break;
			case 'E':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_E] = true;
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
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_W] = false;
				break;
			case 'A':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_A] = false;
				break;
			case 'S':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_S] = false;
				break;
			case 'D':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_D] = false;
				break;
			case 'Q':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_Q] = false;
				break;
			case 'E':
				m_CurrentKeyboardState.m_KeyStates[INPUT_KEY_E] = false;
				break;
			}
		}
		break;
		case WM_MOUSEMOVE:
		{
			uint32_t x = static_cast<uint32_t>(LOWORD(lParam));
			uint32_t y = static_cast<uint32_t>(HIWORD(lParam));
			m_CurrentMouseState.m_MousePosition.x = x;
			m_CurrentMouseState.m_MousePosition.y = y;
		}
		break;
		}
	}

	void InputManager::EndFrame()
	{
		//Reset deltas
		m_CurrentMouseState.m_MouseDelta = Vector2i(0);
		m_CurrentMouseState.m_WheelDelta = 0;
	}

	bool InputManager::IsKeyPressed(InputKey key) const
	{
		return m_CurrentKeyboardState.m_KeyStates.test(key);
	}

	bool InputManager::IsMouseKeyPressed(InputMouseKey mouseKey) const
	{
		return m_CurrentMouseState.m_KeyStates.test(mouseKey);
	}

	const Vector2u& InputManager::GetMousePosition() const
	{
		return m_CurrentMouseState.m_MousePosition;
	}

	const Vector2i& InputManager::GetMouseDelta() const
	{
		return m_CurrentMouseState.m_MouseDelta;
	}

	float InputManager::GetMouseScrollDelta() const
	{
		return m_CurrentMouseState.m_WheelDelta;
	}
}