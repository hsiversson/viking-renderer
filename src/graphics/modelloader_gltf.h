#pragma once
#include "modelloader.h"

namespace vkr::Graphics
{
	class ModelLoader_GLTF : public ModelLoader
	{
	public:
		ModelLoader_GLTF();
		~ModelLoader_GLTF();

		Ref<Model> Load(const std::filesystem::path& filepath) override;

	private:

	};
}