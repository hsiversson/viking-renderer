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
		long m_MouseDeltaX;
		long m_MouseDeltaY;
		short m_WheelDelta;
	};

	class InputManager : public Render::IMessageHandler
	{
	public:
		void ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) override;
		void EndFrame();
		bool IsPressed(InputFlag Flag);
		Vector2f GetMouseDelta() { return { (float)m_CurrentState.m_MouseDeltaX, (float)m_CurrentState.m_MouseDeltaY }; }

	private:
		void ChangeInputState(InputFlag Flag, bool Pressed);

		InputState m_CurrentState;
	};

}