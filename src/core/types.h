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
#include <queue>

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

	template<uint32_t R, uint32_t C>
	class Mat
	{
	public:
		constexpr Mat() = default;
		constexpr Mat(const Mat&) = default;
		constexpr Mat(Mat&&) noexcept = default;
		constexpr Mat<R, C>& operator=(const Mat<R, C>& other) = default;
		constexpr Mat<R, C>& operator=(Mat<R, C>&& other) noexcept = default;

		constexpr Mat(float s) 
		{ 
			std::fill(m, m + (R * C), s); 
		}

		constexpr Mat(std::initializer_list<float> initializerList)
		{
			assert(initializerList.size() == R * C && "Initializer-list length mismatch");
			std::copy(initializerList.begin(), initializerList.end(), m);
		}

		static constexpr Mat Identity()
		{
			Mat m{};
			for (uint32_t i = 0; i < C; ++i)
				m[i * C + i] = 1.0f;
			return m;
		}

		constexpr float* Begin() { return m; }
		constexpr const float* Begin() const { return m; }
		constexpr float* End() { return m + R*C; }
		constexpr const float* End() const { return m + R*C; }

		// std iterator compatibility
		constexpr float* begin() { return Begin(); }
		constexpr const float* begin() const { return Begin(); }
		constexpr float* end() { return End(); }
		constexpr const float* end() const { return End(); }

		constexpr const float& operator[](uint32_t index) const
		{
			assert(index < (R * C));
			return m[index];
		}

		constexpr float& operator[](uint32_t index)
		{
			assert(index < (R * C));
			return m[index];
		}

		constexpr const float& At(uint32_t row, uint32_t col) const
		{
			assert(row < R);
			assert(col < C);
			return m[row * C + col];
		}

		constexpr float& At(uint32_t row, uint32_t col)
		{
			assert(row < R);
			assert(col < C);
			return m[row * C + col];
		}

		constexpr Mat<C, R> GetTransposed() const
		{
			Mat<C, R> t{};
			for (uint32_t r = 0; r < R; ++r)
			{
				for (uint32_t c = 0; c < C; ++c)
				{
					t.At(c, r) = At(r, c);
				}
			}
			return t;
		}

		template<uint32_t RR = R, uint32_t CC = C> requires (RR == CC)
		constexpr Mat& Transpose()
		{
			for (uint32_t i = 0; i < R; ++i)
			{
				for (uint32_t j = i + 1; j < C; ++j)
				{
					std::swap(At(i, j), At(j, i));
				}
			}
			return *this;
		}

		// Specialized Mat<4,3> promotion to Mat<4,4>
		template<std::size_t RR = R, std::size_t CC = C> requires (RR == 4 && CC == 4)
		constexpr Mat(const Mat<4, 3>& a)
			: m{ a[0],  a[1],  a[2],  0.0f,
				 a[3],  a[4],  a[5],  0.0f,
				 a[6],  a[7],  a[8],  0.0f,
				 a[9],  a[10], a[11], 1.0f }
		{}

		float m[R*C];
	};

	using Mat33 = Mat<3, 3>;
	using Mat43 = Mat<4, 3>;
	using Mat44 = Mat<4, 4>;

	template<uint32_t R, uint32_t K, uint32_t C>
	constexpr Mat<R, C>	operator*(const Mat<R, K>& a, const Mat<K, C>& b)
	{
		Mat<R, C> r{};
		for (uint32_t i = 0; i < R; ++i)
		{
			for (uint32_t j = 0; j < C; ++j)
			{
				float sum = 0.0f;
				for (std::size_t k = 0; k < K; ++k)
				{
					sum += a.At(i, k) * b.At(k, j);
				}
				r.At(i, j) = sum;
			}
		}
		return r;
	}

	constexpr Mat<4, 3>
		operator*(const Mat<4, 3>& lhs, const Mat<4, 3>& rhs)
	{
		Mat<4, 3> r{};

		// --- rotation (rows 0-2, 3×3 block) --------------------
		for (std::size_t row = 0; row < 3; ++row)
		{
			r.At(row, 0) = lhs.At(row, 0) * rhs.At(0, 0) + lhs.At(row, 1) * rhs.At(1, 0) + lhs.At(row, 2) * rhs.At(2, 0);
			r.At(row, 1) = lhs.At(row, 0) * rhs.At(0, 1) + lhs.At(row, 1) * rhs.At(1, 1) + lhs.At(row, 2) * rhs.At(2, 1);
			r.At(row, 2) = lhs.At(row, 0) * rhs.At(0, 2) + lhs.At(row, 1) * rhs.At(1, 2) + lhs.At(row, 2) * rhs.At(2, 2);
		}

		// --- translation (row 3) --------------------------------
		r.At(3, 0) = lhs.At(3, 0) * rhs.At(0, 0) + lhs.At(3, 1) * rhs.At(1, 0) + lhs.At(3, 2) * rhs.At(2, 0) + rhs.At(3, 0);
		r.At(3, 1) = lhs.At(3, 0) * rhs.At(0, 1) + lhs.At(3, 1) * rhs.At(1, 1) + lhs.At(3, 2) * rhs.At(2, 1) + rhs.At(3, 1);
		r.At(3, 2) = lhs.At(3, 0) * rhs.At(0, 2) + lhs.At(3, 1) * rhs.At(1, 2) + lhs.At(3, 2) * rhs.At(2, 2) + rhs.At(3, 2);

		return r;
	}

	Mat43 Compose(const Vector3f& m0, const Vector3f& m1, const Vector3f& m2, const Vector3f& m3);
	Mat43 Compose(const Mat33& rotation, const Vector3f& translation);

	Mat44 Compose(const Vector4f& m0, const Vector4f& m1, const Vector4f& m2, const Vector4f& m3);
	Mat44 Compose(const Mat33& rotation, const Vector4f& translation);

	Mat33 CreateRotationX(float angleRadians);
	Mat33 CreateRotationY(float angleRadians);
	Mat33 CreateRotationZ(float angleRadians);

	Mat33 Inverse(const Mat33& m);
	Mat44 Inverse(const Mat44& m);
	Mat44 Inverse(const Mat43& m);

	using ReturnCode = int32_t;
	enum ReturnCodes : int32_t 
	{
		RETURN_OK,
		RETURN_ERROR,
		RETURN_INVALID_ARG,
	};
}

#include "utils/str.h"