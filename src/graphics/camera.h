#pragma once

#include "sceneobject.h"
#include "core/types.h"

namespace vkr::Graphics
{
	class Camera : public SceneObject
	{
	public:
		static constexpr float DefaultNearZ = 0.1f;
		static constexpr float DefaultFarZ = 65000.0f;
		static constexpr float DefaultFov = 80.0f;

	public:
		Camera();
		~Camera() = default;

		void SetSize(const Vector2f& size);
		void SetFov(float fovInDegrees);
		void SetNearZ(float nearZ);
		void SetFarZ(float farZ);
		void SetOrthogonal(bool value);
		void SetInvertedZ(bool value);

		const Vector2f& GetSize() const;
		float GetFov() const;
		float GetAspectRatio() const;
		float GetNearZ() const;
		float GetFarZ() const;

		Mat44 GetView();
		const Mat44& GetProjection() const;
		Mat44 GetViewProjection();

	private:
		void CalculateProjection() const;

		Mat44 m_View;
		mutable Mat44 m_Projection;

		mutable bool m_NeedProjectionUpdate;
		mutable bool m_NeedUpdate;

		Vector2f m_Size;
		float m_FovInDegrees;
		float m_AspectRatio;
		float m_NearZ;
		float m_FarZ;
		bool m_IsOrthogonal;
		bool m_IsInvertedZ;
	};
}