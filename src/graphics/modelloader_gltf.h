#pragma once
#include "modelloader.h"

namespace vkr::Graphics
{
	class ModelLoader_GLTF : public ModelLoader
	{
	public:
		ModelLoader_GLTF();
		~ModelLoader_GLTF();

		bool Load(const std::filesystem::path& filepath) override;

	private:

	};
}