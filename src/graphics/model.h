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

			Render::RaytracingHitGroupDesc GetHitGroupDesc();
		};

	public:
		Model();
		~Model();

		bool Init(const ModelDesc& desc);

		void AddPart(const Part& part);
		const std::vector<Part>& GetParts() const;

		void SetTransform(const Mat44& transform);
		const Mat44& GetTransform() const;

		uint32_t GetTotalNumParts() const;

	private:
		bool InitPart(const ModelDesc::PartDesc& partDesc, Part& outPart);

		Mat44 m_Transform;

		std::vector<Part> m_Parts;
		uint32_t m_TotalNumParts;
	};
}