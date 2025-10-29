#pragma once
#include <memory>

namespace vkr
{
	struct IDeferredDestructibleBase 
	{
		virtual ~IDeferredDestructibleBase() = default;
		virtual void _OnDestroy() = 0;
	};

	template<typename Derived, typename QueueType>
	struct IDeferredDestructible : IDeferredDestructibleBase
	{
		virtual ~IDeferredDestructible() = default;

		virtual void _OnDestroy() 
		{
			delete static_cast<Derived*>(this);
		}

		static QueueType* _GetDeferredDestructionQueue()
		{
			return QueueType::GetInstance();
		}
	};

	template<typename T>
	struct DeferredDestructionTraits 
	{
		static constexpr bool Enabled = std::is_base_of_v<IDeferredDestructibleBase, T>;
	};

	class IDeferredDestructionQueue
	{
	public:
		virtual ~IDeferredDestructionQueue() = default;
		virtual void Enqueue(IDeferredDestructibleBase* obj) = 0;
		virtual void Flush() = 0;
	};

	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T>
	using WeakPtr = std::weak_ptr<T>;

	template<typename T, typename ...Args>
	Ref<T> MakeRef(Args&&... args) 
	{ 
		if constexpr (DeferredDestructionTraits<T>::Enabled)
		{
			return std::shared_ptr<T>(new T(std::forward<Args>(args)...), [](T* ptr)
				{
					IDeferredDestructionQueue* queue = T::_GetDeferredDestructionQueue();
					if (queue)
					{
						T::_GetDeferredDestructionQueue()->Enqueue(ptr);
					}
					else
					{
						ptr->_OnDestroy();
					}
				}
			);
		}
		else 
		{
			return std::make_shared<T>(std::forward<Args>(args)...);
		}
	}

	template<typename T>
	using UniquePtr = std::unique_ptr<T>;

	template<typename T, typename ...Args>
	UniquePtr<T> MakeUnique(Args&&... args) 
	{ 
		return std::make_unique<T>(std::forward<Args>(args)...); 
	}
}