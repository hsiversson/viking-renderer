#pragma once

#if IS_WINDOWS_PLATFORM
#	ifndef NOMINMAX
#		define NOMINMAX
#	endif
#	ifndef WIN32_LEAN_AND_MEAN
#		define WIN32_LEAN_AND_MEAN
#	endif
#	include <windows.h>
#endif

#include "memory.h"
#include "time.h"

namespace vkr
{

}