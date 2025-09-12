#include "quaternion.h"

namespace vkr
{
	Quaternion::Quaternion()
		: w(1.0f)
		, x(0.0f)
		, y(0.0f)
		, z(0.0f)
	{
	}

	Quaternion::Quaternion(float w_, float x_, float y_, float z_)
		: w(w_)
		, x(x_)
		, y(y_)
		, z(z_)
	{
	}

	void Quaternion::Normalize()
	{
		float mag = std::sqrt(w * w + x * x + y * y + z * z);
		if (mag > 0.0f)
		{
			float inv = 1.0f / mag;
			w *= inv;
			x *= inv;
			y *= inv;
			z *= inv;
		}
	}

	Mat44 Quaternion::ToMatrix() const
	{
		Mat44 mat;
		float xx = x * x;
		float yy = y * y;
		float zz = z * z;
		float xy = x * y;
		float xz = x * z;
		float yz = y * z;
		float wx = w * x;
		float wy = w * y;
		float wz = w * z;

		mat.m[0] = 1 - 2 * (yy + zz);  // row 0, col 0
		mat.m[1] = 2 * (xy - wz);      // row 0, col 1
		mat.m[2] = 2 * (xz + wy);      // row 0, col 2
		mat.m[3] = 0;                  // row 0, col 3

		mat.m[4] = 2 * (xy + wz);      // row 1, col 0
		mat.m[5] = 1 - 2 * (xx + zz);  // row 1, col 1
		mat.m[6] = 2 * (yz - wx);      // row 1, col 2
		mat.m[7] = 0;                  // row 1, col 3

		mat.m[8] = 2 * (xz - wy);      // row 2, col 0
		mat.m[9] = 2 * (yz + wx);      // row 2, col 1
		mat.m[10] = 1 - 2 * (xx + yy); // row 2, col 2
		mat.m[11] = 0;                 // row 2, col 3

		mat.m[12] = 0;                 // row 3, col 0
		mat.m[13] = 0;                 // row 3, col 1
		mat.m[14] = 0;                 // row 3, col 2
		mat.m[15] = 1;                 // row 3, col 3

		return mat;
	}

	Vector3f Quaternion::ToEuler() const
	{
		Vector3f euler;

		// Pitch (X)
		float sinr_cosp = 2.0f * (w * x + y * z);
		float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
		euler.x = atan2f(sinr_cosp, cosr_cosp);

		// Yaw (Y)
		float sinp = 2.0f * (w * y - z * x);
		if (fabsf(sinp) >= 1.0f)
			euler.y = copysignf(PI / 2.0f, sinp); // gimbal lock
		else
			euler.y = asinf(sinp);

		// Roll (Z)
		float siny_cosp = 2.0f * (w * z + x * y);
		float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
		euler.z = atan2f(siny_cosp, cosy_cosp);

		return Vector3f(RadToDeg(euler.x), RadToDeg(euler.y), RadToDeg(euler.z));
	}

	Quaternion Quaternion::operator*(const Quaternion& rhs) const
	{
		return Quaternion(
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z,
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w
		);
	}

	bool Quaternion::operator==(const Quaternion& rhs) const
	{
		return (w == rhs.w) && (x == rhs.x) && (y == rhs.y) && (z == rhs.z);
	}

	bool Quaternion::operator!=(const Quaternion& rhs) const
	{
		return !(*this == rhs);
	}

	// Create from axis-angle (angle in radians)
	Quaternion Quaternion::FromAxisAngle(const Vector3f& axis, float angleRad)
	{
		float half = angleRad * 0.5f;
		float s = std::sin(half);
		return Quaternion(
			std::cos(half),
			axis.x * s,
			axis.y * s,
			axis.z * s
		);
	}

	// Create from Euler angles (degrees)
	Quaternion Quaternion::FromEuler(float pitchDeg, float yawDeg, float rollDeg)
	{
		float pRad = DegToRad(pitchDeg);
		float yRad = DegToRad(yawDeg);
		float rRad = DegToRad(rollDeg);

		float cx = cosf(pRad * 0.5f);
		float sx = sinf(pRad * 0.5f);
		float cy = cosf(yRad * 0.5f);
		float sy = sinf(yRad * 0.5f);
		float cz = cosf(rRad * 0.5f);
		float sz = sinf(rRad * 0.5f);

		Quaternion q;
		q.w = cx * cy * cz + sx * sy * sz;
		q.x = sx * cy * cz - cx * sy * sz;
		q.y = cx * sy * cz + sx * cy * sz;
		q.z = cx * cy * sz - sx * sy * cz;
		return q;
	}

	Quaternion Quaternion::FromEuler(const Vector3f& euler)
	{
		return Quaternion::FromEuler(euler.x, euler.y, euler.z);
	}

	Quaternion Quaternion::Identity()
	{
		return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
	}
}