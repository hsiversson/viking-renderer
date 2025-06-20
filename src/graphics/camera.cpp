#include "camera.h"

#include <cassert>

namespace vkr::Graphics
{
	Camera::Camera()
	{
	}

	Camera::~Camera()
	{
	}

	vkr::Mat44 Camera::GetView()
	{
		//Do we need to cache the camera view matrix? Can get expensive if we call it many times. We can override the compute transform from the base class in Camera and recompute the view matrix as well
		Mat44 View;
		assert(Inverse(GetWorldTransform(), View));
		return View;
	}

	vkr::Mat44 Camera::GetProjection() const
	{
		return m_Projection;
	}

	vkr::Mat44 Camera::GetViewProjection()
	{
		return GetView() * GetProjection();
	}

	void Camera::SetupPerspective(float fov, float aspect, float nearZ, float farZ)
	{
		float yscale = 1.0f / tan(fov * 0.5f);
		float xscale = yscale / aspect;

		float q = nearZ / (farZ - nearZ);
		float r = farZ * nearZ / (farZ - nearZ);
		m_Projection =
		{ xscale,0,0,0,
		0,yscale,0,0,
		0,0,q,1,
		0,0,r,0 };
	}

	void Camera::SetupOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ)
	{
		float width = right - left;
		float height = top - bottom;
		float depth = farZ - nearZ; // positive

		float tx = -(left + right) / width;
		float ty = -(top + bottom) / height;
		float tz = -farZ / depth; // inverse depth: near->1, far->0

		m_Projection = {
			2.0f / width,  0.0f,          0.0f,       0.0f,
			0.0f,          2.0f / height, 0.0f,       0.0f,
			0.0f,          0.0f,         -1.0f / depth, 0.0f,
			tx,            ty,           nearZ / depth, 1.0f
		};
	}

}
