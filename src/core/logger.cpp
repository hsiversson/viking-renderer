#include "logger.h"

#if ENABLE_LOGGING
namespace vkr
{
	Logger* Logger::g_Instance = nullptr;

	void Logger::Create()
	{
		assert(g_Instance == nullptr);
		g_Instance = new Logger;
	}

	void Logger::Destroy()
	{
		delete g_Instance;
		g_Instance = nullptr;
	}

	void Logger::QueueMessage(LogMessageType type, const std::string& message, const char* functionName, const char* file, uint32_t lineNumber)
	{
		PendingMessage pendingMessage;
		pendingMessage.m_Type = type;
		pendingMessage.m_Message = message;
		pendingMessage.m_FunctionName = functionName;
		pendingMessage.m_File = file;
		pendingMessage.m_LineNumber = lineNumber;
		pendingMessage.m_Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

		std::unique_lock<std::mutex> lock(g_Instance->m_Mutex);
		g_Instance->m_PendingMessages.push(pendingMessage);
		g_Instance->m_HasWorkEvent.Signal();
	}

	void Logger::QueueMessage(LogMessageType type, const std::wstring& message, const char* functionName /*= nullptr*/, const char* file /*= nullptr*/, uint32_t lineNumber /*= 0*/)
	{
		QueueMessage(type, UTF16ToUTF8(message), functionName, file, lineNumber);
	}

	Logger::Logger()
	{
		m_IsActive = true;
		m_Thread.SetName("Logger Thread");
		m_Thread.Start(&Logger::LoggingFunc, this);
	}

	Logger::~Logger()
	{
		m_IsActive = false; 
		m_HasWorkEvent.Signal();
		m_Thread.Wait();
	}

	void Logger::LoggingFunc()
	{
		while (m_IsActive)
		{
			m_HasWorkEvent.Wait();
			m_HasWorkEvent.Reset();

			while (!m_PendingMessages.empty())
			{
				PendingMessage msg;
				{
					std::unique_lock<std::mutex> lock(m_Mutex);
					msg = m_PendingMessages.front();
					m_PendingMessages.pop();
				}

				const std::string outputString = std::format("{}({}): {}\n", msg.m_File.c_str(), msg.m_LineNumber, msg.m_Message.c_str());
				OutputDebugString(outputString.c_str());
			}
		}
	}
}
#endif // ENABLE_LOGGING