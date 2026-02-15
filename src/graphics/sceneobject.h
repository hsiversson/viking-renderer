#pragma once

#include "core/types.h"

#include "viewrenderdata.h"

namespace vkr::Render
{
	struct Render::RaytracingHitGroupDesc;
}

namespace vkr::Graphics
{
	class Material;

	//Represents any element of a render scene
	class SceneObject : public std::enable_shared_from_this<SceneObject>
	{
	public:
		enum Flags
		{
			Primitive = 0x1
		};

		uint32_t GetFlags() { return m_Flags; }
	protected:

		uint32_t m_Flags = 0;
	};

	//Represents an element of the scene with a 3d representation that needs to go through the normal renderobject path
	//Renderobjects collected for the different passes
	class PrimitiveSceneObject : public SceneObject
	{
	public:
		PrimitiveSceneObject()
		{
			m_Flags = Primitive;
		}

		virtual void CollectRenderObjects(ViewRenderData& renderdata, const std::unordered_map<Material*, uint32_t>& hitGroupLibrary) {}
		virtual void GatherMaterials(std::unordered_set<Material*>& outMaterials) {} //Needed for hit group collection
	};
}