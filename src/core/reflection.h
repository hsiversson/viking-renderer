#pragma once
#include <string>
#include <tuple>
#include <unordered_map>
#include "memory.h"

#define PROPERTY(...)

namespace vkr
{
	template<typename T>
	struct Reflection
	{
		static constexpr bool m_IsReflected = false;
	};

	template<typename T, typename PropertyType>
	struct ReflectedProperty
	{
		constexpr ReflectedProperty(const char* name, PropertyType T::* member) : m_Name(name), m_Member(member) {}

		const char* m_Name;
		PropertyType T::*m_Member;
	};

	struct IReflectionTypeInfo 
	{
		virtual ~IReflectionTypeInfo() = default;
		virtual const char* GetName() const = 0;
	};

	template<typename T>
	struct ReflectionTypeInfo : IReflectionTypeInfo
	{
		const char* GetName() const override { return Reflection<T>::m_TypeName; }
		// Add more metadata here later (size, default instance, etc)
	};

	class ReflectionRegistry
	{
	public:
		static ReflectionRegistry& Get();

		void Register(const char* name, UniquePtr<IReflectionTypeInfo> info);
		IReflectionTypeInfo* Find(const char* name) const;

	private:
		std::unordered_map<std::string, UniquePtr<IReflectionTypeInfo>> m_Types;
	};

	template<typename T, typename Fn>
	constexpr void ForEachProperty(Fn&& func)
	{
		constexpr auto& properties = Reflection<T>::m_Properties;
		std::apply([&](auto&&... property) { (func(property), ...); }, properties);
	}

	template<typename T>
	struct AutoRegister_t
	{
		AutoRegister_t()
		{
			ReflectionRegistry::Get().Register(Reflection<T>::m_TypeName, MakeUnique<ReflectionTypeInfo<T>>());
		}
	};

#define REGISTER_TYPE_REFLECTION(type, id) static AutoRegister_t<type> g_register_type_reflection_##id
}