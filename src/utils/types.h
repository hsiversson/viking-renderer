#pragma once

#include <cstdint>
#include <cfloat>
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
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

	template<uint32_t X, uint32_t Y>
	struct Mat
	{
		float m[X*Y];
	};
	using Mat44 = Mat<4, 4>;
	using Mat33 = Mat<3, 3>;
}

#include "str.h"