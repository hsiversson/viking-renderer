#include "camera.h"

namespace vkr::Graphics
{
	Camera::Camera()
		: m_NeedProjectionUpdate(true)
		, m_NeedUpdate(true)
		, m_Size{256,256}
		, m_FovInDegrees(DefaultFov)
		, m_AspectRatio(1.0f)
		, m_NearZ(DefaultNearZ)
		, m_FarZ(DefaultFarZ)
		, m_IsOrthogonal(false)
		, m_IsInvertedZ(true)
	{
	}

	void Camera::SetSize(const Vector2f& size)
	{
		m_Size = size;
		m_AspectRatio = m_Size.x / m_Size.y;
		m_NeedProjectionUpdate = true;
	}

	void Camera::SetFov(float fovInDegrees)
	{
		m_FovInDegrees = fovInDegrees;
		m_NeedProjectionUpdate = true;
	}

	void Camera::SetNearZ(float nearZ)
	{
		m_NearZ = nearZ;
		m_NeedProjectionUpdate = true;
	}

	void Camera::SetFarZ(float farZ)
	{
		m_FarZ = farZ;
		m_NeedProjectionUpdate = true;
	}

	void Camera::SetOrthogonal(bool value)
	{
		m_IsOrthogonal = value;
		m_NeedProjectionUpdate = true;
	}

	void Camera::SetInvertedZ(bool value)
	{
		m_IsInvertedZ = value;
		m_NeedProjectionUpdate = true;
	}

	const Vector2f& Camera::GetSize() const
	{
		return m_Size;
	}

	float Camera::GetFov() const
	{
		return m_FovInDegrees;
	}

	float Camera::GetAspectRatio() const
	{
		return m_AspectRatio;
	}

	float Camera::GetNearZ() const
	{
		return m_NearZ;
	}

	float Camera::GetFarZ() const
	{
		return m_FarZ;
	}

	Mat44 Camera::GetView()
	{
		//Do we need to cache the camera view matrix? Can get expensive if we call it many times. We can override the compute transform from the base class in Camera and recompute the view matrix as well
		return Inverse(GetWorldTransform());
	}

	const Mat44& Camera::GetProjection() const
	{
		if (m_NeedProjectionUpdate)
		{
			CalculateProjection();
		}
		return m_Projection;
	}

	Mat44 Camera::GetViewProjection()
	{
		return GetView() * GetProjection();
	}

	void Camera::CalculateProjection() const
	{
		if (m_IsOrthogonal)
		{
			const float zScale = 1.0f / (m_FarZ - m_NearZ);
			const float zOffset = -m_NearZ;
			const float width = m_Size.x * 0.5f;
			const float height = m_Size.y * 0.5f;
			if (m_IsInvertedZ)
			{
				m_Projection = {
					(width != 0.0f) ? 1.0f / width : 1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, (height != 0.0f) ? 1.0f / height : 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, -zScale, 0.0f,
					0.0f, 0.0f, 1.0f - zOffset * zScale, 1.0f
				};
			}
			else
			{
				m_Projection = {
					(width != 0.0f) ? 1.0f / width : 1.0f, 0.0f, 0.0f, 0.0f,
					0.0f, (height != 0.0f) ? 1.0f / height : 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, zScale, 0.0f,
					0.0f, 0.0f, zOffset * zScale, 1.0f
				};
			}
		}
		else
		{
			const float halfFov = DegToRad(m_FovInDegrees * 0.5f);
			if (m_IsInvertedZ)
			{
				m_Projection = {
					1.0f / tanf(halfFov), 0.0f, 0.0f, 0.0f,
					0.0f, m_Size.x / tanf(halfFov) / m_Size.y, 0.0f, 0.0f,
					0.0f, 0.0f, ((m_NearZ == m_FarZ) ? 0.0f : m_NearZ / (m_NearZ - m_FarZ)), 1.0f,
					0.0f, 0.0f, ((m_NearZ == m_FarZ) ? m_NearZ : -m_FarZ * m_NearZ / (m_NearZ - m_FarZ)), 0.0f
				};
			}
			else
			{
				m_Projection = {
					1.0f / tanf(halfFov), 0.0f, 0.0f, 0.0f,
					0.0f, m_Size.x / tanf(halfFov) / m_Size.y, 0.0f, 0.0f,
					0.0f, 0.0f, ((m_NearZ == m_FarZ) ? (1.0f) : m_FarZ / (m_FarZ - m_NearZ)), 1.0f,
					0.0f, 0.0f, -m_NearZ * ((m_NearZ == m_FarZ) ? (1.0f) : m_FarZ / (m_FarZ - m_NearZ)), 0.0f
				};
			}
		}
		m_NeedProjectionUpdate = false;
	}
}
