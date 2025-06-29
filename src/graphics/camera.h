#pragma once

#include "sceneobject.h"
#include "core/types.h"

namespace vkr::Graphics
{
	class Camera : public SceneObject
	{
	public:
		Camera();
		~Camera();

		Mat44 GetView();
		Mat44 GetProjection() const;
		Mat44 GetViewProjection();
		//fov in radians, aspect as width/height, near > 0 , far > near
		void SetupPerspective(float fov, float aspect, float nearZ, float farZ);
		void SetupOrthographic(float left, float right, float bottom, float top, float nearZ, float farZ);

	private:
		Mat44 m_View;
		Mat44 m_Projection; //Left hand, row major with inverse depth
	};
}