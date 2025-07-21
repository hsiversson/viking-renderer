#pragma once

#if defined(_WIN64)
#	define IS_WINDOWS_PLATFORM	1
#elif defined(__linux__)
#	define IS_LINUX_PLATFORM	1
#endif

#if defined(BUILD_CONFIG_SHIPPING)
#	define ENABLE_LOGGING		0
#	define ENABLE_PROFILING		0
#	define ENABLE_CONSOLE		0
#	define ENABLE_EDITOR		0
#else
#	define ENABLE_LOGGING		1
#	define ENABLE_PROFILING		1
#	define ENABLE_CONSOLE		1
#	define ENABLE_EDITOR		1
#endif

#if !defined(IS_WINDOWS_PLATFORM)
#	define IS_WINDOWS_PLATFORM	0
#endif

#if !defined(IS_LINUX_PLATFORM)
#	define IS_LINUX_PLATFORM	0
#endif