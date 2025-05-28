#include "types.h"

namespace vkr
{
	const Mat44 Mat44::Identity = { 1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0 };
	const Mat43 Mat43::Identity = { 1,0,0,0,1,0,0,0,1,0,0,0 };
	const Mat33 Mat33::Identity = { 1,0,0,0,1,0,0,0,1 };

	vkr::Mat43 Mat43::operator*(const Mat43& other)
	{
		Mat43 result;
		// Rotation part (3 rows × 3 columns)
		for (int row = 0; row < 3; ++row)
		{
			int r = row * 3;
			result[r + 0] = m[r + 0] * other[0] + m[r + 1] * other[3] + m[r + 2] * other[6];
			result[r + 1] = m[r + 0] * other[1] + m[r + 1] * other[4] + m[r + 2] * other[7];
			result[r + 2] = m[r + 0] * other[2] + m[r + 1] * other[5] + m[r + 2] * other[8];
		}

		// Translation row
		result[9]  = m[9] * other[0] + m[10] * other[3] + m[11] * other[6] + other[9];
		result[10] = m[9] * other[1] + m[10] * other[4] + m[11] * other[7] + other[10];
		result[11] = m[9] * other[2] + m[10] * other[5] + m[11] * other[8] + other[11];

		return result;
	}

	Mat44::Mat44(const Mat43& other)
	{
		m[0] = other[0]; m[1] = other[1]; m[2] = other[2]; m[3] = 0;
		m[4] = other[3]; m[5] = other[4]; m[6] = other[5]; m[7] = 0;
		m[8] = other[6]; m[9] = other[7]; m[10] = other[8]; m[11] = 0;
		m[12] = other[9]; m[13] = other[10]; m[14] = other[11]; m[15] = 1;
	}

	Mat44::Mat44()
	{
	}

	Mat44::Mat44(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20, float m21, float m22, float m23, float m30, float m31, float m32, float m33)
	{
		m[0] = m00; m[1] = m01; m[2] = m02; m[3] = m03;
		m[4] = m10; m[5] = m11; m[6] = m12; m[7] = m13;
		m[8] = m20; m[9] = m21; m[10] = m22; m[11] = m23;
		m[12] = m30; m[13] = m31; m[14] = m32; m[15] = m33;
	}

	Mat44 Mat44::operator*(const Mat44& other)
	{
		Mat44 result;
		for (int row = 0; row < 4; ++row)
		{
			for (int col = 0; col < 4; ++col)
			{
				result[row * 4 + col] =
					m[row * 4 + 0] * other[0 * 4 + col] +
					m[row * 4 + 1] * other[1 * 4 + col] +
					m[row * 4 + 2] * other[2 * 4 + col] +
					m[row * 4 + 3] * other[3 * 4 + col];
			}
		}
		return result;
	}

	bool Inverse(const Mat44& m, Mat44& out)
	{
		Mat44 inv;

		inv[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		inv[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		inv[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		inv[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		inv[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		inv[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		inv[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		inv[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		inv[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		inv[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		inv[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		inv[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +
			m[12] * m[2] * m[5];

		inv[3] = -m[1] * m[6] * m[11] +
			m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -
			m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +
			m[9] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] -
			m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +
			m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -
			m[8] * m[3] * m[6];

		inv[11] = -m[0] * m[5] * m[11] +
			m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -
			m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +
			m[8] * m[3] * m[5];

		inv[15] = m[0] * m[5] * m[10] -
			m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +
			m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -
			m[8] * m[2] * m[5];

		float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

		if (fabs(det) < 1e-6f)
			return false; // not invertible

		float invDet = 1.0f / det;
		for (int i = 0; i < 16; i++)
			out[i] = inv[i] * invDet;

		return true;
	}

}