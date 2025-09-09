#pragma once

#include "globaldefines.h"

#define VKR_CONCAT_IMPL(x, y) x##y
#define VKR_CONCAT(x, y) VKR_CONCAT_IMPL(x, y)

namespace vkr
{
	static constexpr float PI = 3.1415927410125732421875f;

	inline constexpr float DegToRad(float degree) { return degree * (PI / 180.0f); }
	inline constexpr float RadToDeg(float radians) { return radians * (180.0f / PI); }

	inline constexpr uint8_t  Align(uint8_t  value, uint8_t  alignment) { return (value + alignment - 1) & ~(alignment - 1); }
	inline constexpr uint16_t Align(uint16_t value, uint16_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
	inline constexpr uint32_t Align(uint32_t value, uint32_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }
	inline constexpr uint64_t Align(uint64_t value, uint64_t alignment) { return (value + alignment - 1) & ~(alignment - 1); }

	template<typename T>
	inline constexpr T Saturate(const T& v) { return std::clamp(v, T(0), T(1)); }
}

#include "types.h"
#include "platform.h"
#include "random.h"
#include "utils/str.h"
#include "logger.h"
#include "thread.h"