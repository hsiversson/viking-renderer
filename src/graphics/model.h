#pragma once
#include "utils/types.h"

namespace vkr::Graphics
{
	class Mesh;
	class Material;

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

		void AddPart(const Part& part);
		const std::vector<Part>& GetParts() const;

	private:
		std::vector<Part> m_Parts;
	};
}