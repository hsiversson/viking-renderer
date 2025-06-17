#include "texture.h"

#include "device.h"

static const D3D12_HEAP_PROPERTIES DefHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

namespace vkr::Render
{
	Texture::Texture()
	{

	}

	Texture::~Texture()
	{

	}

	bool Texture::Init(const TextureDesc& desc, const TextureData* initialData /*= nullptr*/)
	{
		m_TextureDesc = desc;

		//For now allocate resource in place. Later well see how we do pooling
		UINT16 MipLevels = 1;
		if (desc.bUseMips)
		{
			int32_t MaxDim = std::max<int32_t>(desc.Size.x, desc.Size.y);
			MipLevels = unsigned int(std::floor(std::log2(MaxDim))) + 1;
		}

		ID3D12Resource* resource;
		D3D12_RESOURCE_DESC TextureDesc;
		TextureDesc.Dimension = desc.Dimension == 1 ? D3D12_RESOURCE_DIMENSION_TEXTURE1D : (desc.Dimension == 2 ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE3D);
		TextureDesc.Format = desc.Format;
		TextureDesc.Width = desc.Size.x;
		TextureDesc.Height = desc.Size.y;
		TextureDesc.DepthOrArraySize = desc.Dimension > 2 ? desc.Size.z : desc.ArraySize;
		TextureDesc.MipLevels = MipLevels;
		TextureDesc.Alignment = 0;
		D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.bWriteable)
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		if (desc.bDepthStencil)
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		TextureDesc.Flags = Flags;
		TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		TextureDesc.SampleDesc.Count = 1;
		TextureDesc.SampleDesc.Quality = 0;
		//If we want the texture to be used as UAV assume the initial state then will be UAV access
		HRESULT hr = GetDevice().GetD3DDevice()->CreateCommittedResource(&DefHeapProps, D3D12_HEAP_FLAG_NONE, &TextureDesc, desc.bWriteable ? D3D12_RESOURCE_STATE_UNORDERED_ACCESS : D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
		if (FAILED(hr))
		{
			return false;
		}

		//Fill with initial data (missing)

		m_Resource = resource;
		return true;
	}

}