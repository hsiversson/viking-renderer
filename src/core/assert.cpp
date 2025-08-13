#include "assert.h"

#include <cstdarg>
#include <format>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

namespace vkr
{
	bool Assert(const char* file, int32_t line, const char* cond, const char* formattedString)
	{		
		const std::string outputString = std::format("{}(line {})\n\nAssert failed: ({})\n{}\n", file, line, cond, formattedString ? formattedString : "");
		const std::string titleString = std::format("Assert Failed - {}", cond);

#if IS_WINDOWS_PLATFORM
		OutputDebugString(outputString.c_str());
		int32_t result = MessageBox(nullptr, outputString.c_str(), titleString.c_str(), (MB_CANCELTRYCONTINUE | MB_ICONERROR | MB_DEFBUTTON2));
		if (result == IDCANCEL)
		{
			PostQuitMessage(3);
			return false;
		}
		else if (result == IDTRYAGAIN)
		{
			return true;
		}
		else
		{
			return false;
		}

#else
#	error Platform not supported yet!
#endif

		return false;
	}
}

#undef ARRAY_SIZE