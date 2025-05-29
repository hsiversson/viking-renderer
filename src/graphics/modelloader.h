#pragma once
#include "utils/types.h"

namespace vkr::Graphics
{
	class ModelLoader
	{
	public:
		virtual bool Load(const std::filesystem::path& filepath) = 0;

	protected:

	};
}