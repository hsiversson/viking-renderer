#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	struct TextureDesc
	{
		int Dimension;
		Vector3i Size;
		int ArraySize;
		DXGI_FORMAT Format;
		bool bUseMips;
	};

	class Texture
	{
	public:
		Texture();
		~Texture();

		bool Init(ID3D12Resource* Resource);

		// How do we handle resource views? (SRV, UAV)
		// Separate class, or do we include here?
		// Each resource can have multiple views.

	private:
		ComPtr<ID3D12Resource> m_Resource; // do we generalize the resources between texture/buffer? We need to keep track of resource state etc, so would be annoying to duplicate.
	};
}