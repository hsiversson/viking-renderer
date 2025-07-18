#pragma once
#include "core/types.h"
#include "graphics/mesh.h"
#include "graphics/material.h"

namespace vkr::Graphics
{
	struct ModelDesc
	{
		struct PartDesc
		{
			MeshDesc m_MeshDesc;
			MaterialDesc m_MaterialDesc;
			Mat44 m_LocalTransform;

			std::vector<PartDesc> m_ChildDescs;
		};

		std::vector<PartDesc> m_PartDescs;
	};

	class Model
	{
	public:
		struct Part
		{
			Ref<Mesh> m_Mesh;
			Ref<MaterialInstance> m_Material;
			Mat44 m_LocalTransform;
			std::vector<Part> m_ChildParts;
		};

	public:
		Model();
		~Model();

		bool Init(const ModelDesc& desc);

		void AddPart(const Part& part);
		const std::vector<Part>& GetParts() const;

	private:
		bool InitPart(const ModelDesc::PartDesc& partDesc, Part& outPart);

		std::vector<Part> m_Parts;
	};
}