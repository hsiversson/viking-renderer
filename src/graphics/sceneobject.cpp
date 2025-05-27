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
		auto parent = m_Parent.lock();
		if (parent)
		{
			auto ParentTransform = parent->GetWorldTransform();
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

	void SceneObject::AddChild(Ref<SceneObject> child)
	{
		if (!child)
			return; 

		auto it = std::find(m_Children.begin(), m_Children.end(), child);
		if (it == m_Children.end())
		{
			child->m_Parent = shared_from_this();
			m_Children.push_back(child);
		}
	}

	void SceneObject::RemoveChild(Ref<SceneObject> child)
	{
		if (!child)
			return;
		m_Children.erase(std::remove(m_Children.begin(), m_Children.end(), child), m_Children.end());
	}

}