#pragma once

#include "core/types.h"
#include "model.h"

namespace vkr::Render
{
	class Shader;
	class PipelineState;
}

namespace vkr::Graphics
{
	class Sky;
	class View;
	class ViewManager;
	class ViewRenderer;
	class ViewRenderData;
	class Scene
	{
	public:
		Scene();
		~Scene();

		View* CreateView();
		void DestroyView(View*);

		// Run per frame updates to the scene and its objects & views
		void Update();

		void AddModel(const Ref<Model>& model);
		void RemoveModel(const Ref<Model>& model);

	private:
		// Prepare render data for rendering for each view. 
		// I.e extract renderable information and store in list to be picked up by render tasks later
		void PrepareView(View* view);

		void CollectModelPart(ViewRenderData& renderData, const Model::Part& part, const Mat44& parentWorldTransform, const Mat44& prevParentWorldTransform);

		// For now only a simple list of scene objects, 
		// but later maybe a spatial partitioning structure of scene objects?
		// Quadtree, Octree, Grid?
		std::vector<Ref<Model>> m_Models;
		//std::vector<Ref<LocalLight>> m_LocalLights;

		struct PendingAction
		{
			enum class Type
			{
				Add,
				Remove,
			};
			enum class ObjectType
			{
				Model,
				LocalLight,
			};
			Type m_Type;
			ObjectType m_ObjectType;
			Ref<Model> m_Model;
			//Ref<Light> m_LocalLight;
		};

		std::mutex m_PendingActionsMutex;
		std::queue<PendingAction> m_PendingActions;

		UniquePtr<ViewManager> m_ViewManager;
		UniquePtr<ViewRenderer> m_ViewRenderer;

		UniquePtr<Sky> m_Sky;

		uint32_t m_MaterialHitGroupCounter = 0;
		std::unordered_map<Material*, uint32_t> m_MaterialToHitGroupId;
		std::vector<Render::RaytracingHitGroupDesc> m_HitGroupDescs;
		Ref<Render::Shader> m_TraceRaysDynamicShaderLib;
		Ref<Render::PipelineState> m_TraceRaysPipelineState;
	};
}