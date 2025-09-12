#include "threadpool.h"

namespace vkr
{
	ThreadPool* ThreadPool::g_Instance = nullptr;

	void ThreadPool::Create()
	{
		VKR_ASSERT(g_Instance == nullptr);
		g_Instance = new ThreadPool;
		g_Instance->Init(std::thread::hardware_concurrency() - 1);
	}

	void ThreadPool::Destroy()
	{
		delete g_Instance;
		g_Instance = nullptr;
	}

	ThreadPool& ThreadPool::Get()
	{
		VKR_ASSERT(g_Instance != nullptr);
		return *g_Instance;
	}

	ThreadPool::~ThreadPool()
	{
		{
			std::unique_lock<std::mutex> lock(m_Mutex);
			m_Stop = true;
		}
		m_Condition.notify_all();
		m_Workers.clear();
	}

	bool ThreadPool::Init(uint32_t numThreads)
	{
		m_Workers.resize(numThreads);

		m_Stop = false;
		for (uint32_t i = 0; i < numThreads; ++i)
		{
			std::string name = std::format("Task Thread {}", i);
			m_Workers[i].SetName(name.c_str());
			m_Workers[i].Start(&ThreadPool::WorkerLoop, this);
		}

		return true;
	}
	
	void ThreadPool::WaitForShortTasks()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Condition.wait(lock, [this]() { return m_PendingShortTasks.empty(); });
	}

	void ThreadPool::WorkerLoop()
	{
		while (true)
		{
			Task task;
			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				m_Condition.wait(lock, [this]() { return m_Stop || !m_PendingShortTasks.empty() || !m_PendingLongTasks.empty(); });

				if (m_Stop)
					return;

				if (!m_PendingShortTasks.empty())
				{
					task = std::move(m_PendingShortTasks.front());
					m_PendingShortTasks.pop();
				}
				else if (!m_PendingLongTasks.empty())
				{
					task = std::move(m_PendingLongTasks.front());
					m_PendingLongTasks.pop();
				}
			}

			if (task)
			{
				task();
			}
		}
	}
}