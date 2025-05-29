#pragma once

namespace vkr::Graphics
{
	class ModelLoader_GLTF
	{
	public:
		ModelLoader_GLTF();
		~ModelLoader_GLTF();

		bool Load(const std::filesystem::path& filepath);

	private:

	};
}