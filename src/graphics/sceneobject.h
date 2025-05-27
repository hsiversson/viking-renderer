#pragma once

#include "utils/types.h"

namespace vkr::Graphics
{
	class SceneObject
	{
	public:
		Mat43 GetWorldTransform();
		void SetLocalTransform(const Mat43& Local);
		const Mat43& GetLocalTransform() const;
	private:
		void ComputeTransform();

		Ref<SceneObject> m_Parent = nullptr;
		std::vector<Ref<SceneObject>> m_Children;
		// For now we will put the transform on the scene object base class. Maybe in the future its more advantageous to create a subclass
		// like TransformSceneObject in case we want to have objects on the scene that have no spatial representation or meaning
		Mat43 m_Local = Mat43::Identity;
		Mat43 m_World = Mat43::Identity;
		bool m_TransformDirty = false; //Lazy evaluation of final transform
	};
}