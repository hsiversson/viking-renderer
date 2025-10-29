#include "reflection.h"

namespace vkr
{
	ReflectionRegistry& ReflectionRegistry::Get()
	{
		static ReflectionRegistry instance;
		return instance;
	}

	void ReflectionRegistry::Register(std::string_view name, UniquePtr<IReflectionTypeInfo> info)
	{
		m_Types[name] = std::move(info);
	}

	IReflectionTypeInfo* ReflectionRegistry::Find(std::string_view name) const
	{
		if (auto it = m_Types.find(name); it != m_Types.end())
		{
			return it->second.get();
		}
		return nullptr;
	}
}