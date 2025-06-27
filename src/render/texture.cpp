#include "texture.h"

#include "device.h"
#include "d3dconvert.h"

static constexpr D3D12_HEAP_PROPERTIES g_DefaultHeapProps{ D3D12_HEAP_TYPE_DEFAULT, D3D12_CPU_PAGE_PROPERTY_UNKNOWN, D3D12_MEMORY_POOL_UNKNOWN, 1, 1 };

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
		uint16_t mipLevels = 1;
		if (desc.m_CalculateMips)
		{
			uint32_t maxDim = std::max(desc.m_Size.x, desc.m_Size.y);
			mipLevels = std::floor(std::log2(maxDim)) + 1;
		}

		D3D12_RESOURCE_DESC1 textureDesc = {};

		switch (desc.m_Dimension)
		{
		case ResourceDimension::Texture1D:
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE1D;
			break;
		case ResourceDimension::Texture2D:
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			break;
		case ResourceDimension::Texture3D:
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
			break;
		default:
			assert(false);
			return false;
		}

		textureDesc.Format = D3DConvertFormat(desc.m_Format);
		textureDesc.Width = desc.m_Size.x;
		textureDesc.Height = desc.m_Size.y;
		textureDesc.DepthOrArraySize = desc.m_Dimension == ResourceDimension::Texture3D ? desc.m_Size.z : desc.m_ArraySize;
		textureDesc.MipLevels = mipLevels;
		textureDesc.Alignment = 0;
		textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		if (desc.m_Writeable)
		{
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}
		if (desc.m_AllowRenderTarget)
		{
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}
		if (desc.m_AllowDepthStencil)
		{
			textureDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		}

		D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_COMMON;
		m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_COMMON;
		m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_COMMON;
		m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		if (desc.m_Writeable)
		{
			initialLayout = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
			m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_WRITE;
			m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_READ_WRITE_RESOURCE;
			m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		}
		else if (desc.m_AllowDepthStencil)
		{
			initialLayout = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
			m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_DEPTH_WRITE;
			m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_DEPTH_STENCIL;
			m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		}

		D3D12_CLEAR_VALUE optimizedClearValue;
		if (desc.m_AllowRenderTarget || desc.m_AllowDepthStencil)
		{
			optimizedClearValue.Format = textureDesc.Format;
			optimizedClearValue.Color[0] = 0.0f;
			optimizedClearValue.Color[1] = 0.0f;
			optimizedClearValue.Color[2] = 0.0f;
			optimizedClearValue.Color[3] = 0.0f;
			optimizedClearValue.DepthStencil = { 0.0f, 0 };
		}

		HRESULT hr = GetDevice().GetD3DDevice10()->CreateCommittedResource3(&g_DefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, initialLayout, (desc.m_AllowRenderTarget || desc.m_AllowDepthStencil) ? &optimizedClearValue : nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&m_Resource));
		if (FAILED(hr))
		{
			return false;
		}

		//Fill with initial data (missing)
		if (initialData)
		{

		}

		return true;
	}

	bool Texture::InitWithResource(const TextureDesc& desc, const ComPtr<ID3D12Resource>& resource, const ResourceStateTracking& initialState)
	{
		m_Resource = resource;
		m_TextureDesc = desc;
		m_StateTracking = initialState;
		return true;
	}

}