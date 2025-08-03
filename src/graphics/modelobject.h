#pragma once

#include "sceneobject.h"
#include "core/types.h"
#include "graphics/model.h"

namespace vkr::Graphics
{
	//Represents an instance of a model on the scene with its own transform and properties
	class ModelObject final : public SceneObject
	{
	public:
		ModelObject();
		~ModelObject();

		void SetModel(Ref<class Model> model) { m_Model = model; }

		void CollectRenderObjects(ViewRenderData& renderData) override; 
		void CollectRaytracingHitGroups(std::vector<Render::RaytracingHitGroupDesc>& outHitGroups) override;

	private:
		void CollectModelPart(uint32_t& partCounter, ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform);
		void CollectRaytracingHitGroup(std::vector<Render::RaytracingHitGroupDesc>& outHitGroups, const Model::Part& part);

		Ref<Model> m_Model;
		uint32_t m_MaterialHitGroupIndexOffset;
	};
}