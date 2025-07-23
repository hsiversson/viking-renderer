#pragma once

#include "sceneobject.h"
#include "core/types.h"
#include "graphics/model.h"

namespace vkr::Graphics
{
	//Represents an instance of a model on the scene with its own transform and properties
	class ModelObject : public SceneObject
	{
	public:
		ModelObject();
		~ModelObject();

		void SetModel(Ref<class Model> model) { m_Model = model; }

		void CollectRenderObjects(ViewRenderData& renderData) override;

	private:
		void CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform);

		Ref<Model> m_Model;
	};
}