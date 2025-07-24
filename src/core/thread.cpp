#include "thread.h"
#include "utils/str.h"

namespace vkr
{
#if IS_WINDOWS_PLATFORM
	static void SetThreadName(void* nativeThreadHandle, const std::string& name)
	{
		const std::wstring wName = UTF8ToUTF16(name);
		SetThreadDescription(static_cast<HANDLE>(nativeThreadHandle), wName.c_str());
	}
#endif


	thread_local bool Thread::g_IsMainThread = false;
	thread_local bool Thread::g_IsRenderThread = false;
	thread_local Thread* Thread::g_CurrentThread = nullptr;

	Thread::Thread()
		: m_Name("Unnamed thread")
	{

	}

	Thread::~Thread()
	{
		Wait();
	}

	void Thread::Wait()
	{
		if (m_Thread.joinable())
			m_Thread.join();
	}

	const char* Thread::GetName() const
	{
		return m_Name.c_str();
	}

	void Thread::SetName(const char* name)
	{
		m_Name = name;
		if (m_Thread.joinable())
			SetThreadName(m_Thread.native_handle(), m_Name);
	}

	bool Thread::IsActive() const
	{
		return m_Thread.joinable();
	}

	void Thread::Yield()
	{
		std::this_thread::yield();
	}

	void Thread::Sleep(uint32_t milliseconds)
	{
		if (milliseconds == 0)
		{
			Yield();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
		}
	}

	static std::thread::id g_MainThreadId;
	void Thread::RegisterMainThread()
	{
		if (g_MainThreadId == std::thread::id())
		{
			g_MainThreadId = std::this_thread::get_id();
			g_IsMainThread = true;
		}
	}

	void Thread::RegisterRenderThread()
	{
		g_IsRenderThread = true;
	}

	bool Thread::IsMainThread()
	{
		return g_IsMainThread;
	}

	bool Thread::IsRenderThread()
	{
		return g_IsRenderThread;
	}
}


