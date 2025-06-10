#pragma once

#include "utils/types.h"

#include "viewrenderdata.h"

namespace vkr::Graphics
{
	class SceneObject : public std::enable_shared_from_this<SceneObject>
	{
	public:
		Mat43 GetWorldTransform();
		void SetLocalTransform(const Mat43& Local);
		const Mat43& GetLocalTransform() const;
		void AddChild(Ref<SceneObject> child);
		void RemoveChild(Ref<SceneObject> child);

		//Temporal render mechanism
		virtual void CollectRenderObjects(ViewRenderData& renderdata) {}
	private:
		void ComputeTransform();

		WeakPtr<SceneObject> m_Parent;
		std::vector<Ref<SceneObject>> m_Children;
		// For now we will put the transform on the scene object base class. Maybe in the future its more advantageous to create a subclass
		// like TransformSceneObject in case we want to have objects on the scene that have no spatial representation or meaning
		Mat43 m_Local = Mat43::Identity;
		Mat43 m_World = Mat43::Identity;
		bool m_TransformDirty = false; //Lazy evaluation of final transform
	};
}