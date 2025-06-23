#pragma once
#include "core/types.h"

namespace vkr::Render
{
	class Window
	{
	public:
		Window(const char* name, const Vector2u& size, int32_t showCmd);
		~Window();

		bool PeekMessages();

		void* GetNativeHandle() const;

	private:
		void* m_NativeHandle;
	};
}