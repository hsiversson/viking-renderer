#include "types.h"

namespace vkr
{
	Mat43 Compose(const Vector3f& m0, const Vector3f& m1, const Vector3f& m2, const Vector3f& m3)
	{
		Mat43 m;
		m[0] = m0.x; m[1] = m0.y; m[2] = m0.z;
		m[3] = m1.x; m[4] = m1.y; m[5] = m1.z;
		m[6] = m2.x; m[7] = m2.y; m[8] = m2.z;
		m[9] = m3.x; m[10] = m3.y; m[11] = m3.z;
		return m;
	}
	Mat43 Compose(const Mat33& rotation, const Vector3f& translation)
	{
		Mat43 m;
		std::copy(rotation.m, rotation.m + 9, m.m);
		m[9] = translation.x; m[10] = translation.y; m[11] = translation.z;
		return m;
	}

	Mat44 Compose(const Vector4f& m0, const Vector4f& m1, const Vector4f& m2, const Vector4f& m3)
	{
		Mat44 m;
		m[0]  = m0.x; m[1]  = m0.y; m[2]  = m0.z; m[3]  = m0.w;
		m[4]  = m1.x; m[5]  = m1.y; m[6]  = m1.z; m[7]  = m1.w;
		m[8]  = m2.x; m[9]  = m2.y; m[10] = m2.z; m[11] = m2.w;
		m[12] = m3.x; m[13] = m3.y; m[14] = m3.z; m[15] = m3.w;
		return m;
	}

	Mat44 Compose(const Mat33& rotation, const Vector4f& translation)
	{
		Mat44 m;
		m[0] = rotation.At(0, 0); m[1] = rotation.At(0, 1); m[2]  = rotation.At(0, 2), m[3]  = 0.0f;
		m[4] = rotation.At(1, 0); m[5] = rotation.At(1, 0); m[6]  = rotation.At(1, 0), m[7]  = 0.0f;
		m[8] = rotation.At(2, 0); m[9] = rotation.At(2, 0); m[10] = rotation.At(2, 0), m[11] = 0.0f;
		m[12] = translation.x; m[13] = translation.y; m[14] = translation.z, m[15] = translation.w;
		return m;
	}

	Mat33 CreateRotationX(float angleRadians)
	{
		float s = std::sinf(angleRadians);
		float c = std::cosf(angleRadians);
		return Mat33{
			1.0f, 0.0f, 0.0f,
			0.0f, c, s,
			0.0f, -s, c
		};
	}

	Mat33 CreateRotationY(float angleRadians)
	{
		const float s = std::sinf(angleRadians);
		const float c = std::cosf(angleRadians);

		return Mat33{
			  c , 0.0f,  s ,
			0.0f, 1.0f, 0.0f,
			 -s , 0.0f,  c
		};
	}

	Mat33 CreateRotationZ(float angleRadians)
	{
		const float s = std::sinf(angleRadians);
		const float c = std::cosf(angleRadians);

		return Mat33{
			c, s, 0.0f,
			-s, c, 0.0f,
			0.0f, 0.0f, 1.0f
		};
	}

	Mat33 Inverse(const Mat33& m)
	{
		float c00 = m[4] * m[8] - m[5] * m[7];
		float c01 = -(m[3] * m[8] - m[5] * m[6]);
		float c02 = m[3] * m[7] - m[4] * m[6];

		float c10 = -(m[1] * m[8] - m[2] * m[7]);
		float c11 = m[0] * m[8] - m[2] * m[6];
		float c12 = -(m[0] * m[7] - m[1] * m[6]);

		float c20 = m[1] * m[5] - m[2] * m[4];
		float c21 = -(m[0] * m[5] - m[2] * m[3]);
		float c22 = m[0] * m[4] - m[1] * m[3];

		float det = m[0] * c00 + m[1] * c01 + m[2] * c02;
		assert(std::fabs(det) > 1e-7f && "Matrix is singular, no inverse.");

		float invDet = 1.0f / det;
		return Mat33{
			c00 * invDet, c10 * invDet, c20 * invDet,
			c01 * invDet, c11 * invDet, c21 * invDet,
			c02 * invDet, c12 * invDet, c22 * invDet
		};
	}

	Mat44 Inverse(const Mat44& m)
	{
		float inv[16];

		inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15]
			+ m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];

		inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15]
			- m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];

		inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15]
			+ m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];

		inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11]
			- m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];

		inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15]
			- m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];

		inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15]
			+ m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];

		inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15]
			- m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11]
			+ m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];

		inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15]
			+ m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];

		inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15]
			- m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];

		inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15]
			+ m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];

		inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11]
			- m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];

		inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14]
			- m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];

		inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14]
			+ m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];

		inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14]
			- m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];

		inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10]
			+ m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

		float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
		assert(std::fabs(det) > 1e-7f && "Matrix not invertible");

		float invDet = 1.0f / det;

		Mat44 invMat{};
		for (int i = 0; i < 16; i++)
			invMat[i] = inv[i] * invDet;

		return invMat;
	}
	Mat44 Inverse(const Mat43& m)
	{
		Mat44 i{
			m[0], m[1], m[2], 0.0f,
			m[3], m[4], m[5], 0.0f,
			m[6], m[7], m[8], 0.0f,
			m[9], m[10], m[11], 1.0f
		};

		return Inverse(i);
	}
}