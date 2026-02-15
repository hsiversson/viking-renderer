#pragma once

#include "sceneobject.h"
#include "core/types.h"
#include "graphics/model.h"

namespace vkr::Graphics
{
	//Represents an instance of a model on the scene with its own transform and properties
	class ModelSceneObject final : public PrimitiveSceneObject
	{
	public:
		ModelSceneObject();
		~ModelSceneObject();

		void SetModel(Ref<class Model> model) { m_Model = model; }
		Ref<Model> GetModel() { return m_Model; }

		void CollectRenderObjects(ViewRenderData& renderData, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary) override;
		void GatherMaterials(std::unordered_set<Material*>& outMaterials) override;

	private:
		void CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform);

		Ref<Model> m_Model;
		uint32_t m_MaterialHitGroupIndexOffset;
	};
}