#pragma once

#include <cstdint>
#include <cfloat>
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <numbers>
#include <algorithm>
#include <cassert>
#include <memory>
#include <mutex>

namespace vkr
{
	template<typename T>
	using Ref = std::shared_ptr<T>;

	template<typename T>
	using WeakPtr = std::weak_ptr<T>;

	template<typename T, typename ...Args>
	Ref<T> MakeRef(Args&&... args) { return std::make_shared<T>(std::forward<Args>(args)...); }

	template<typename T>
	using UniquePtr = std::unique_ptr<T>;

	template<typename T, typename ...Args>
	UniquePtr<T> MakeUnique(Args&&... args) { return std::make_unique<T>(std::forward<Args>(args)...); }

	template<typename T>
	struct Vector2
	{
		T x, y;
	};
	using Vector2f = Vector2<float>;
	using Vector2i = Vector2<int32_t>;
	using Vector2u = Vector2<uint32_t>;

	template<typename T>
	struct Vector3
	{
		T x, y, z;
	};
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

	template<uint32_t X, uint32_t Y>
	struct Mat
	{
		float operator[](size_t Idx) const
		{
			return m[Idx];
		}

		float& operator[](size_t Idx)
		{
			return m[Idx];
		}

		Mat<X, Y>& operator=(const Mat<X, Y>& other)
		{
			std::copy(other.m, other.m + (X * Y), m);
			return *this;
		}

		float m[X*Y];
	};

	struct Mat43 : public Mat<4, 3>
	{
		static const Mat43 Identity;
		Mat43() {};
		Mat43(float m00, float m01, float m02,
			float m10, float m11, float m12,
			float m20, float m21, float m22,
			float m30, float m31, float m32);
		Mat43(const struct Mat33& rot, const Vector3f& translation);
		Mat43 operator*(const Mat43& other);
	};

	struct Mat44 : public Mat<4, 4>
	{
		static const Mat44 Identity;

		Mat44();

		Mat44(float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33);

		Mat44(const Mat43& other);

		Mat44 operator*(const Mat44& other);
	};

	bool Inverse(const Mat44& m, Mat44& out);

	struct Mat33 : public Mat<3, 3>
	{
		static const Mat33 Identity;

		Mat33(float m00, float m01, float m02,
			float m10, float m11, float m12,
			float m20, float m21, float m22);
		static Mat33 CreateRotationZ(float angleradians);
	};
	
}

#include "str.h"