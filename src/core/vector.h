#pragma once

namespace vkr
{
	template<typename T>
	struct Vector2
	{
		T x, y;

		template<typename U>
		explicit operator Vector2<U>() const
		{
			return Vector2<U>(
				static_cast<U>(x),
				static_cast<U>(y)
			);
		}

		bool operator==(const Vector2& other) const { return x == other.x && y == other.y; }
		bool operator!=(const Vector2& other) const { return !(*this == other); }
	};
	using Vector2f = Vector2<float>;
	using Vector2i = Vector2<int32_t>;
	using Vector2u = Vector2<uint32_t>;

	template<typename T>
	Vector2<T> operator-(const Vector2<T>& v, float scalar)
	{
		return { v.x - scalar, v.y - scalar };
	}

	template<typename T>
	Vector2<T> operator*(const Vector2<T>& v, float scalar)
	{
		return { v.x * scalar, v.y * scalar };
	}

	template<typename T>
	Vector2<T> operator*(const Vector2<T>& v0, const Vector2<T>& v1)
	{
		return { v0.x * v1.x, v0.y * v1.y };
	}

	template<typename T>
	Vector2<T> operator/(const Vector2<T>& v0, const Vector2<T>& v1)
	{
		return { v0.x / v1.x, v0.y / v1.y };
	}

	template<typename T>
	struct Vector3
	{
		T x, y, z;
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