#pragma once
#include "core/matrix.h"

namespace vkr
{
	class Quaternion;
	class Rotator
	{
	public:
		float m_Pitch;
		float m_Yaw;
		float m_Roll;

	public:
		Rotator();
		Rotator(float pitch, float yaw, float roll);
		Rotator(Vector3f v);

		Quaternion ToQuaternion() const;
		void FromQuaternion(const Quaternion& q);

		Mat33 ToMat33() const;
		void FromMat33(const Mat33& m);

		Mat44 ToMat44() const;
		void FromMat44(const Mat44& m);

		float& operator[](uint32_t index);
		const float& operator[](uint32_t index) const;

		bool operator==(const Rotator&) const = default;
		bool operator!=(const Rotator&) const = default;
	};
}