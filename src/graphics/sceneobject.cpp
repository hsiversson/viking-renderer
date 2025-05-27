#include "sceneobject.h"


namespace vkr::Graphics
{
	Mat43 SceneObject::GetWorldTransform()
	{
		if (m_TransformDirty)
		{
			ComputeTransform();
			m_TransformDirty = false;
		}

		return m_World;
	}

	void SceneObject::ComputeTransform()
	{
		if (m_Parent)
		{
			auto ParentTransform = m_Parent->GetWorldTransform();
			m_World = m_Local * ParentTransform;
		}
	}

	void SceneObject::SetLocalTransform(const Mat43& Local)
	{
		m_Local = Local;
		m_TransformDirty = true;
	}

	const vkr::Mat43& SceneObject::GetLocalTransform() const
	{
		return m_Local;
	}

}