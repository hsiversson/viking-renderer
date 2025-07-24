#pragma once 
#include <thread>
#include <string>

#ifdef Yield
#undef Yield
#endif

namespace vkr
{
	class Thread
	{
	public:
		Thread();
		~Thread();

		template<typename Fn, typename ...Args>
		void Start(Fn&& func, Args&&... args)
		{
			m_Thread = std::thread(std::forward<Fn>(func), std::forward<Args>(args)...);
			SetName(m_Name.c_str());
		}

		void Wait();

		const char* GetName() const;
		void SetName(const char* name);

		bool IsActive() const;

		static void Yield();
		static void Sleep(uint32_t milliseconds);

		static void RegisterMainThread();
		static void RegisterRenderThread();
		static bool IsMainThread();
		static bool IsRenderThread();

	private:
		std::thread m_Thread;
		std::string m_Name;

		static thread_local bool g_IsMainThread;
		static thread_local bool g_IsRenderThread;
		static thread_local Thread* g_CurrentThread;
	};
}