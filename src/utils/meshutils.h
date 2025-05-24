#pragma once

#include "graphics/mesh.h"

namespace vkr::Render
{
	class Device;
}

namespace vkr
{
	Ref<Graphics::Mesh> CreateCubeMesh(Ref<Render::Device> device);
}