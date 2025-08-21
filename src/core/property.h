#pragma once

namespace vkr
{
	enum class PropertyFlags : uint32_t
	{
		None = 0,
		Editable = 1 << 0,
		ReadOnly = 1 << 1,
	};

	inline PropertyFlags operator|(PropertyFlags a, PropertyFlags b)
	{
		return static_cast<PropertyFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline bool HasFlag(PropertyFlags flags, PropertyFlags test)
	{
		return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(test)) != 0;
	}

	template<typename T>
	class Property
	{
	public:
		Property(const T& value = {}, std::string name = std::string(), PropertyFlags flags = PropertyFlags::None)
			: m_Name(std::move(name))
			, m_Flags(flags) 
			, m_Value(value)
		{
		}

		operator const T& () const { return m_Value; }
		operator T& () { return m_Value; }

		Property& operator=(const T& value) { m_Value = value; return *this; }

		const std::string& GetName() const { return m_Name; }
		PropertyFlags GetFlags() const { return m_Flags; }

	private:
		std::string m_Name;
		PropertyFlags m_Flags;
		T m_Value{};
	};
}

#define PROPERTY(type, name, ...) Property<type> m_##name { __VA_ARGS__ }