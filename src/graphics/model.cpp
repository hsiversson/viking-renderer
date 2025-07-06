#include "model.h"
#include "core/types.h"
#include "mesh.h"
#include "material.h"

namespace vkr::Graphics
{
	Model::Model()
	{

	}

	Model::~Model()
	{

	}

	bool Model::Init(const ModelDesc& desc)
	{
		m_Parts.reserve(desc.m_PartDescs.size());
		for (uint32_t i = 0; i < desc.m_PartDescs.size(); ++i)
		{
			const ModelDesc::PartDesc& partDesc = desc.m_PartDescs[i];

			Part part;
			part.m_Mesh = MakeRef<Mesh>();
			if (!part.m_Mesh->Init(partDesc.m_MeshDesc))
			{
				return false;
			}

			part.m_Material = MakeRef<Material>();
			if (!part.m_Material->Init(partDesc.m_MaterialDesc))
			{
				return false;
			}

			m_Parts.push_back(part);
		}

		return true;
	}

	void Model::AddPart(const Part& part)
	{
		m_Parts.push_back(part);
	}

	const std::vector<Model::Part>& Model::GetParts() const
	{
		return m_Parts;
	}

}