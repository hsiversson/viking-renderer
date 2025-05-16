#pragma once

namespace vkr::Render
{
	struct TextureDesc
	{

	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		bool Init(const TextureDesc& desc);

		// How do we handle resource views? (SRV, UAV)
		// Separate class, or do we include here?
		// Each resource can have multiple views.

	private:
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}