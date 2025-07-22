#pragma once

#include "core/types.h"
#include "render/window.h"

#include <bitset>

namespace vkr
{
	enum InputFlag : uint16_t 
	{
		//Mouse
		MouseLeft = 0,
		MouseRight,
		MouseMiddle,
		MouseX1,
		MouseX2,

		//Keyboard
		KeyW,
		KeyA,
		KeyS,
		KeyD,
		KeyQ,
		KeyE
	};

	struct InputState
	{
		std::bitset<512> m_InputFlags;
		Vector2u m_MousePosition;
		Vector2i m_MouseDelta;
		float m_WheelDelta;
	};

	class InputManager : public Render::IMessageHandler
	{
	public:
		void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
		void EndFrame();
		bool IsPressed(InputFlag Flag);
		Vector2f GetMousePosition() const { return { (float)m_CurrentState.m_MousePosition.x, (float)m_CurrentState.m_MousePosition.y }; }
		Vector2f GetMouseDelta() const { return { (float)m_CurrentState.m_MouseDelta.x, (float)m_CurrentState.m_MouseDelta.y }; }
		float GetMouseScrollDelta() const { return m_CurrentState.m_WheelDelta; }

	private:
		void ChangeInputState(InputFlag Flag, bool Pressed);

		InputState m_CurrentState;
	};

}