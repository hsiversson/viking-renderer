#pragma once
#include "thread.h"
#include <future>
#include <queue>

namespace vkr
{
	template<typename T>
	using Future = std::future<T>;

	class ThreadPool
	{
	public:
		using Task = std::function<void()>;

		enum class TaskType
		{
			Short,
			Long
		};

		static void Create();
		static void Destroy();
		static ThreadPool& Get();

		ThreadPool() = default;
		~ThreadPool();

		bool Init(uint32_t numThreads);

		void WaitForShortTasks();

		template<typename F, typename... Args>
		auto QueueTask(TaskType type, F&& f, Args&&... args) -> Future<typename std::invoke_result_t<F, Args...>>
		{
			using ReturnType = typename std::invoke_result_t<F, Args...>;

			auto task = std::make_shared<std::packaged_task<ReturnType()>>(
				std::bind(std::forward<F>(f), std::forward<Args>(args)...)
			);

			Future<ReturnType> future = task->get_future();
			std::queue<Task>& targetQueue = type == TaskType::Short ? m_PendingShortTasks : m_PendingLongTasks;

			{
				std::unique_lock<std::mutex> lock(m_Mutex);
				if (m_Stop)
				{
					throw std::runtime_error("QueueTask on stopped ThreadPool");
				}
				targetQueue.emplace([task]() { (*task)(); });
			}
			m_Condition.notify_one();

			return future;
		}

	private:
		void WorkerLoop();

	private:
		std::vector<Thread> m_Workers;
		std::mutex m_Mutex;
		std::condition_variable m_Condition;
		std::queue<Task> m_PendingShortTasks;
		std::queue<Task> m_PendingLongTasks;

		std::atomic<bool> m_Stop = false;

		static ThreadPool* g_Instance;
	};

	template<typename F, typename... Args>
	auto CreateShortTask(F&& fn, Args&&... args)
	{
		return ThreadPool::Get().QueueTask(ThreadPool::TaskType::Short, std::forward<F>(fn), std::forward<Args>(args)...);
	}

	template<typename F, typename... Args>
	auto CreateLongTask(F&& fn, Args&&... args)
	{
		return ThreadPool::Get().QueueTask(ThreadPool::TaskType::Long, std::forward<F>(fn), std::forward<Args>(args)...);
	}
}