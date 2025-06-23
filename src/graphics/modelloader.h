#pragma once
#include "core/types.h"

namespace vkr::Graphics
{
	class Model;
	class ModelLoader
	{
	public:
		virtual Ref<Model> Load(const std::filesystem::path& filepath) = 0;

	protected:

	};
}