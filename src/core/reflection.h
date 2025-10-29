#pragma once
#include <string_view>
#include <tuple>
#include <unordered_map>
#include "memory.h"

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
		constexpr ReflectedProperty(std::string_view name, PropertyType T::* member) : m_Name(name), m_Member(member) {}

		std::string_view m_Name;
		PropertyType T::*m_Member;
	};

	struct IReflectionTypeInfo 
	{
		virtual ~IReflectionTypeInfo() = default;
		virtual std::string_view GetName() const = 0;
	};

	template<typename T>
	struct ReflectionTypeInfo : IReflectionTypeInfo
	{
		std::string_view GetName() const override { return Reflection<T>::m_TypeName; }
		// Add more metadata here later (size, default instance, etc)
	};

	class ReflectionRegistry
	{
	public:
		static ReflectionRegistry& Get();

		void Register(std::string_view name, UniquePtr<IReflectionTypeInfo> info);
		IReflectionTypeInfo* Find(std::string_view name) const;

	private:
		std::unordered_map<std::string_view, UniquePtr<IReflectionTypeInfo>> m_Types;
	};

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