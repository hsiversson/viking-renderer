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

		// Row-major rotation matrix elements
		float r00 = 1 - 2 * (y * y + z * z);
		float r01 = 2 * (x * y - z * w);
		float r02 = 2 * (x * z + y * w);

		float r10 = 2 * (x * y + z * w);
		float r11 = 1 - 2 * (x * x + z * z);
		float r12 = 2 * (y * z - x * w);

		float r20 = 2 * (x * z - y * w);
		float r21 = 2 * (y * z + x * w);
		float r22 = 1 - 2 * (x * x + y * y);

		// Pitch (X)
		euler.x = RadToDeg(asinf(-r21));

		// Handle gimbal lock
		if (fabsf(r21) < 0.999999f)
		{
			// Yaw (Y) and Roll (Z)
			euler.y = RadToDeg(atan2f(r20, r22));
			euler.z = RadToDeg(atan2f(r01, r11));
		}
		else
		{
			euler.y = RadToDeg(atan2f(-r02, r00));
			euler.z = 0.0f;
		}

		return euler;
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
		float pitch = DegToRad(pitchDeg);
		float yaw = DegToRad(yawDeg);
		float roll = DegToRad(rollDeg);

		float cy = std::cos(yaw * 0.5f);
		float sy = std::sin(yaw * 0.5f);
		float cp = std::cos(pitch * 0.5f);
		float sp = std::sin(pitch * 0.5f);
		float cr = std::cos(roll * 0.5f);
		float sr = std::sin(roll * 0.5f);

		Quaternion q;
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;
		return q;
	}
}