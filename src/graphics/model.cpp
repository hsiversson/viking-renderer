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
			Part part;
			InitPart(desc.m_PartDescs[i], part);
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

	bool Model::InitPart(const ModelDesc::PartDesc& partDesc, Part& outPart)
	{
		outPart.m_LocalTransform = partDesc.m_LocalTransform;

		outPart.m_Mesh = MakeRef<Mesh>();
		if (!outPart.m_Mesh->Init(partDesc.m_MeshDesc))
		{
			return false;
		}

		Ref<Material> material = MakeRef<Material>();
		if (!material->Init(partDesc.m_MaterialDesc))
		{
			return false;
		}
		outPart.m_Material = MakeRef<MaterialInstance>(material);

		outPart.m_ChildParts.reserve(partDesc.m_ChildDescs.size());
		for (uint32_t i = 0; i < partDesc.m_ChildDescs.size(); ++i)
		{
			Part childPart = {};
			InitPart(partDesc.m_ChildDescs[i], childPart);
			outPart.m_ChildParts.push_back(childPart);
		}

		return true;
	}

}