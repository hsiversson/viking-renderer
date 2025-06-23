#pragma once

#include "sceneobject.h"
#include "core/types.h"

namespace vkr::Graphics
{
	//Represents an instance of a model on the scene with its own transform and properties
	class ModelObject : public SceneObject
	{
	public:
		ModelObject();
		~ModelObject();

		void SetModel(Ref<class Model> model) { m_Model = model; }

		void CollectRenderObjects(ViewRenderData& renderdata) override;

	private:
		Ref<class Model> m_Model;
	};
}