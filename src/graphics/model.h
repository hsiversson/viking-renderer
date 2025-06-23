#pragma once
#include "core/types.h"
#include "graphics/mesh.h"
#include "graphics/material.h"

namespace vkr::Graphics
{
	struct ModelDesc
	{
		std::vector<MeshDesc> m_MeshDescs;
		std::vector<MaterialDesc> m_MaterialDescs;
	};

	class Model
	{
	public:
		struct Part
		{
			Ref<Mesh> m_Mesh;
			Ref<Material> m_Material;
		};

	public:
		Model();
		~Model();

		bool Init(const ModelDesc& desc);

		void AddPart(const Part& part);
		const std::vector<Part>& GetParts() const;

	private:
		bool InitMeshes(const ModelDesc& desc);
		bool InitMaterials(const ModelDesc& desc);

	private:
		std::vector<Part> m_Parts;
	};
}