#include "model.h"
#include "utils/types.h"
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

	void Model::AddPart(const Part& part)
	{
		m_Parts.push_back(part);
	}

	const std::vector<Model::Part>& Model::GetParts() const
	{
		return m_Parts;
	}

}