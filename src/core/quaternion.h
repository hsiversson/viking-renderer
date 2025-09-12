#pragma once

namespace vkr
{
	class Quaternion
	{
	public:
		float w, x, y, z;

	public:
		Quaternion();
		Quaternion(float w_, float x_, float y_, float z_);

		void Normalize();

		Mat44 ToMatrix() const;
		Vector3f ToEuler() const;

		Quaternion operator*(const Quaternion& rhs) const;

		bool operator==(const Quaternion& rhs) const;
		bool operator!=(const Quaternion& rhs) const;

		static Quaternion FromAxisAngle(const Vector3f& axis, float angleRad);
		static Quaternion FromEuler(float pitchDeg, float yawDeg, float rollDeg);
		static Quaternion FromEuler(const Vector3f& euler);
		static Quaternion Identity();
	};
}