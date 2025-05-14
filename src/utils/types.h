#pragma once

#include <cstdint>
#include <cfloat>

namespace vkr
{
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

	template<uint32_t X, uint32_t Y>
	struct Mat
	{
		float m[X*Y];
	};
	using Mat44 = Mat<4, 4>;
	using Mat33 = Mat<3, 3>;
}