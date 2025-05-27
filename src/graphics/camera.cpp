#include "camera.h"

#include <cassert>

namespace vkr::Graphics
{
	void Camera::SetupPerspective(float fov, float aspect, float near, float far)
	{
		float yscale = 1.0f / tan(fov * 0.5f);
		float xscale = yscale / aspect;

		float q = near / (near - far);
		float r = far * near / (near - far);
		m_Projection = 
		{xscale,0,0,0,
		0,yscale,0,0,
		0,0,q,1,
		0,0,r,0};
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

	void Camera::SetupOrthographic(float left, float right, float bottom, float top, float near, float far)
	{
		float width = right - left;
		float height = top - bottom;
		float depth = far - near; // positive

		float tx = -(left + right) / width;
		float ty = -(top + bottom) / height;
		float tz = -far / depth; // inverse depth: near->1, far->0

		m_Projection = {
			2.0f / width,  0.0f,          0.0f,       0.0f,
			0.0f,          2.0f / height, 0.0f,       0.0f,
			0.0f,          0.0f,         -1.0f / depth, 0.0f,
			tx,            ty,           near / depth, 1.0f
		};
	}

}
