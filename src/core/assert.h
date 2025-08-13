#pragma once

#include "globaldefines.h"

#include <cassert>
#undef assert

#if IS_WINDOWS_PLATFORM
#	define VKR_DEBUG_BREAK() __debugbreak()
#else
#	error Platform not supported yet.
#endif

namespace vkr
{
	bool Assert(const char* file, int32_t line, const char* cond, const char* formattedString);
	inline bool Assert(const char* file, int32_t line, const char* cond)
	{
		return Assert(file, line, cond, nullptr);
	}

	template<typename Fmt, typename... Args>
	inline bool Assert(const char* file, int32_t line, const char* cond, Fmt&& fmtString, Args&&... args)
	{
		std::string message;
		if constexpr (sizeof...(Args) == 0)
		{
			message = std::string(fmtString);
		}
		else
		{
			message = std::vformat(fmtString, std::make_format_args(args...));
		}

		return Assert(file, line, cond, message.c_str());
	}
}

#define VKR_ASSERT_ENSURE(cond, ...)										\
	do 																		\
	{																		\
		if (!(cond))														\
		{																	\
			if (vkr::Assert(__FILE__, __LINE__, (#cond), ##__VA_ARGS__))	\
			{																\
				VKR_DEBUG_BREAK();											\
			}																\
		}																	\
	} while (false)

#define VKR_ASSERT(cond, ...) VKR_ASSERT_ENSURE(cond, ##__VA_ARGS__)
#define VKR_CHECK_NO_ENTRY() VKR_ASSERT(false, "Enclosing block should never be called")