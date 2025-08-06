#pragma once

namespace vkr
{
	template<typename T>
	struct Vector2
	{
		T x, y;

		constexpr Vector2() = default;
		constexpr ~Vector2() = default;

		template<typename U>
		constexpr Vector2(U scalar)
			: x(static_cast<T>(scalar))
			, y(static_cast<T>(scalar))
		{
		}

		template<typename U>
		constexpr Vector2(U x0, U y0)
			: x(static_cast<T>(x0))
			, y(static_cast<T>(y0))
		{
		}

		template<typename U>
		explicit constexpr Vector2(const Vector2<U>& other)
			: x(static_cast<T>(other.x))
			, y(static_cast<T>(other.y))
		{
		}

		template<typename U>
		explicit constexpr operator Vector2<U>() const
		{
			return Vector2<U>(static_cast<U>(x), static_cast<U>(y));
		}

		constexpr bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
		constexpr bool operator!=(const Vector2& other) const { return !(*this == other); }
	};
	using Vector2f = Vector2<float>;
	using Vector2i = Vector2<int32_t>;
	using Vector2u = Vector2<uint32_t>;

	template<typename T>
	constexpr Vector2<T> operator-(const Vector2<T>& v, float scalar)
	{
		return { v.x - scalar, v.y - scalar };
	}

	template<typename T>
	constexpr Vector2<T> operator-(const Vector2<T>& v0, const Vector2<T>& v1)
	{
		return { v0.x - v1.x, v0.y - v1.y };
	}

	template<typename T>
	constexpr Vector2<T> operator*(const Vector2<T>& v, float scalar)
	{
		return { v.x * scalar, v.y * scalar };
	}

	template<typename T>
	constexpr Vector2<T> operator*(const Vector2<T>& v0, const Vector2<T>& v1)
	{
		return { v0.x * v1.x, v0.y * v1.y };
	}

	template<typename T>
	constexpr Vector2<T> operator/(const Vector2<T>& v0, const Vector2<T>& v1)
	{
		return { v0.x / v1.x, v0.y / v1.y };
	}

	template<typename T>
	constexpr Vector2<T> operator/(const Vector2<T>& v0, float scalar)
	{
		return { v0.x / scalar, v0.y / scalar };
	}

	template<typename T>
	constexpr Vector2<T> operator/(float scalar, const Vector2<T>& v0)
	{
		return { scalar / v0.x, scalar / v0.y };
	}

	template<typename T>
	struct Vector3
	{
		T x, y, z;

		constexpr bool operator==(const Vector3& other) const { return x == other.x && y == other.y && z == other.z; }
		constexpr bool operator!=(const Vector3& other) const { return !(*this == other); }
	};

	template<typename T>
	Vector3<T> operator*(float scalar, Vector3<T> v)
	{
		return { scalar * v.x, scalar * v.y, scalar + v.z };
	}

	template<typename T>
	Vector3<T> operator*(Vector3<T> v, float scalar)
	{
		return { scalar * v.x, scalar * v.y, scalar * v.z };
	}

	template<typename T>
	Vector3<T> operator+(Vector3<T> v, Vector3<T> w)
	{
		return { v.x + w.x, v.y + w.y, v.z + w.z };
	}

	template<typename T>
	Vector3<T> operator-(Vector3<T> v, Vector3<T> w)
	{
		return { v.x - w.x, v.y - w.y, v.z - w.z };
	}

	template<typename T>
	Vector3<T> operator/(Vector3<T> v, float scalar)
	{
		return { v.x / scalar, v.y / scalar, v.z / scalar };
	}

	template<typename T>
	float Length(const Vector3<T>& v)
	{
		return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	}

	template<typename T>
	void Normalize(Vector3<T>& v)
	{
		v = Normalized(v);
	}

	template<typename T>
	Vector3<T> Normalized(const Vector3<T>& v)
	{
		return v / Length(v);
	}

	template<typename T>
	Vector3<T> Cross(const Vector3<T>& a, const Vector3<T>& b)
	{
		return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
	}

	template<typename T>
	Vector3<T> Lerp(const Vector3<T>& a, const Vector3<T>& b, T t)
	{
		return a * (T(1) - t) + b * t;
	}

	using Vector3f = Vector3<float>;
	using Vector3i = Vector3<int32_t>;
	using Vector3u = Vector3<uint32_t>;

	template<typename T>
	struct Vector4
	{
		T x, y, z, w;
	};
	using Vector4f = Vector4<float>;
	using Vector4i = Vector4<int32_t>;
	using Vector4u = Vector4<uint32_t>;
	using Vector4u16 = Vector4<uint16_t>;
}