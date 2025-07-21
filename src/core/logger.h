#pragma once

#if ENABLE_LOGGING
#include <thread>
#include <mutex>
#include <queue>
#include <format>
#include "core/event.h"

namespace vkr
{
	enum LogMessageType
	{
		LOG_MESSAGE_TYPE_INFO,
		LOG_MESSAGE_TYPE_WARNING,
		LOG_MESSAGE_TYPE_ERROR,
	};

	class Logger
	{
		struct PendingMessage
		{
			std::string m_Message;
			std::string m_FunctionName;
			std::string m_File;
			uint32_t m_LineNumber = 0;
			std::time_t m_Time = 0;
			LogMessageType m_Type = LOG_MESSAGE_TYPE_INFO;
		};

	public:
		static void Create();
		static void Destroy();
		static Logger* Get() { return g_Instance; }

		static void QueueMessage(LogMessageType type, const std::string& message, const char* functionName = nullptr, const char* file = nullptr, uint32_t lineNumber = 0);
		static void QueueMessage(LogMessageType type, const std::wstring& message, const char* functionName = nullptr, const char* file = nullptr, uint32_t lineNumber = 0);

	private:
		Logger();
		~Logger();

		void LoggingFunc();

		std::mutex m_Mutex;
		std::thread m_Thread;
		std::queue<PendingMessage> m_PendingMessages;
		Event m_HasWorkEvent;

		bool m_IsActive;

		static Logger* g_Instance;
	};
}

#define VKR_LOG(msg, ...)		vkr::Logger::Get()->QueueMessage(LOG_MESSAGE_TYPE_INFO, std::format(msg, ##__VA_ARGS__), __FUNCTION__, __FILE__, __LINE__);
#define VKR_WARNING(msg, ...)	vkr::Logger::Get()->QueueMessage(LOG_MESSAGE_TYPE_WARNING, std::format(msg, ##__VA_ARGS__), __FUNCTION__, __FILE__, __LINE__);
#define VKR_ERROR(msg, ...)		vkr::Logger::Get()->QueueMessage(LOG_MESSAGE_TYPE_ERROR, std::format(msg, ##__VA_ARGS__), __FUNCTION__, __FILE__, __LINE__);

#else // ENABLE_LOGGING

#define VKR_LOG(msg, ...)
#define VKR_WARNING(msg, ...)
#define VKR_ERROR(msg, ...)

#endif // ENABLE_LOGGING