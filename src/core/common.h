#pragma once

#include <cassert>

#define checkNoEntry() assert(false && "Enclosing block should never be called")

namespace vkr
{
	static constexpr uint64_t Align(uint64_t value, uint64_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}
}