#include "rotator.h"

namespace vkr
{
	Rotator::Rotator()
		: m_Pitch(0.0f)
		, m_Yaw(0.0f)
		, m_Roll(0.0f)
	{
	}

	Rotator::Rotator(float pitch, float yaw, float roll)
		: m_Pitch(pitch)
		, m_Yaw(yaw)
		, m_Roll(roll)
	{
	}

	Rotator::Rotator(Vector3f v)
		: m_Pitch(v.x)
		, m_Yaw(v.y)
		, m_Roll(v.z)
	{
	}

	Quaternion Rotator::ToQuaternion() const
	{
		return Quaternion::FromEuler(m_Pitch, m_Yaw, m_Roll);
	}

	void Rotator::FromQuaternion(const Quaternion& q)
	{
		Vector3f v = q.ToEuler();
		m_Pitch = v.x;
		m_Yaw = v.y;
		m_Roll = v.z;
	}

	Mat33 Rotator::ToMat33() const
	{
		return Mat33();
	}

	void Rotator::FromMat33(const Mat33& m)
	{
	}

	Mat44 Rotator::ToMat44() const
	{
		return Mat44();
	}

	void Rotator::FromMat44(const Mat44& m)
	{
	}

	float& Rotator::operator[](uint32_t index)
	{
		return index == 0 ? m_Pitch : (index == 1) ? m_Yaw : m_Roll;
	}

	const float& Rotator::operator[](uint32_t index) const
	{
		return index == 0 ? m_Pitch : (index == 1) ? m_Yaw : m_Roll;
	}
}