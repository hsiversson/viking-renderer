#include "material.h"
#include "render/texture.h"
#include "render/pipelinestate.h"

namespace vkr::Graphics
{

	Material::Material()
	{

	}

	Material::~Material()
	{

	}

	void Material::AddTexture(const Ref<Render::TextureView>& tex)
	{
		m_Textures.push_back(tex);
	}

	Render::TextureView* Material::GetTexture() const
	{
		if (m_Textures.empty())
			return nullptr;

		return m_Textures[0].get();
	}

}