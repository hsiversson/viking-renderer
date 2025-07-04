#include "resourcedescriptor.h"
#include "device.h"
#include "d3dconvert.h"
#include "utils/hash.h"

namespace vkr::Render
{
	static DescriptorHeap* GetHeap(ResourceDescriptorType type)
	{
		switch (type)
		{
		case RESOURCE_DESCRIPTOR_TYPE_TEXTURE_VIEW:
		case RESOURCE_DESCRIPTOR_TYPE_BUFFER_VIEW:
			return GetDevice().GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SHADER_RESOURCE);
		case RESOURCE_DESCRIPTOR_TYPE_SAMPLER:
			return GetDevice().GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_SAMPLER);
		case RESOURCE_DESCRIPTOR_TYPE_RENDER_TARGET_VIEW:
			return GetDevice().GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_RENDER_TARGET);
		case RESOURCE_DESCRIPTOR_TYPE_DEPTH_STENCIL_VIEW:
			return GetDevice().GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE_DEPTH_STENCIL);
		default:
			return nullptr;
		}
	}

	ResourceDescriptor::ResourceDescriptor(ResourceDescriptorType type)
		: m_D3DHandle{}
		, m_DescriptorIndex(g_InvalidIndex)
		, m_DescHash(0)
		, m_Type(type)
	{
	}

	ResourceDescriptor::~ResourceDescriptor()
	{
		if (m_DescriptorIndex != g_InvalidIndex)
		{
			FreeDescriptor();
		}
	}

	bool ResourceDescriptor::AllocateDescriptor()
	{
		DescriptorHeap* heap = GetHeap(m_Type);
		if (heap)
		{
			heap->Allocate(*this);
			return true;
		}
		return false;
	}

	bool ResourceDescriptor::FreeDescriptor()
	{
		DescriptorHeap* heap = GetHeap(m_Type);
		if (heap)
		{
			heap->Release(*this);
			return true;
		}
		return false;
	}

	TextureView::TextureView()
		: ResourceDescriptor(RESOURCE_DESCRIPTOR_TYPE_TEXTURE_VIEW)
	{
	}

	bool TextureView::Init(const TextureViewDesc& desc, const Ref<Texture>& resource)
	{
		if (AllocateDescriptor())
		{
			const TextureDesc& textureDesc = resource->m_TextureDesc;

			if (desc.m_Writable)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = D3DConvertFormat(textureDesc.m_Format);
				if (textureDesc.m_Dimension == ResourceDimension::Texture1D)
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
					uavDesc.Texture1D.MipSlice = desc.m_Mip;
				}
				else if (textureDesc.m_Dimension == ResourceDimension::Texture2D)
				{
					uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					uavDesc.Texture2D.PlaneSlice = 0;
					uavDesc.Texture2D.MipSlice = desc.m_Mip;
				}
				GetDevice().GetD3DDevice()->CreateUnorderedAccessView(resource->GetD3DResource(), nullptr, &uavDesc, m_D3DHandle);
			}
			else
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = D3DConvertFormat(textureDesc.m_Format);
				srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				if (textureDesc.m_Dimension == ResourceDimension::Texture1D)
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
					srvDesc.Texture1D.MipLevels = textureDesc.m_MipLevels;
					srvDesc.Texture1D.MostDetailedMip = desc.m_Mip;
				}
				else if (textureDesc.m_Dimension == ResourceDimension::Texture2D)
				{
					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.PlaneSlice = 0;
					srvDesc.Texture2D.MipLevels = textureDesc.m_MipLevels;
					srvDesc.Texture2D.MostDetailedMip = desc.m_Mip;
					srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
				}
				GetDevice().GetD3DDevice()->CreateShaderResourceView(resource->GetD3DResource(), &srvDesc, m_D3DHandle);
			}
			m_Texture = resource;
			m_DescHash = hash_fnv64(reinterpret_cast<const uint8_t*>(&desc), sizeof(desc));
			m_Texture->TrackDescriptor(m_DescHash, weak_from_this());
			return true;
		}
		return false;
	}

	RenderTargetView::RenderTargetView()
		: ResourceDescriptor(RESOURCE_DESCRIPTOR_TYPE_RENDER_TARGET_VIEW)
	{
	}

	bool RenderTargetView::Init(const RenderTargetViewDesc& desc, const Ref<Texture>& resource)
	{
		if (AllocateDescriptor())
		{
			// TODO: add desc
			GetDevice().GetD3DDevice()->CreateRenderTargetView(resource->GetD3DResource(), nullptr, m_D3DHandle);

			m_Texture = resource;
			m_DescHash = hash_fnv64(reinterpret_cast<const uint8_t*>(&desc), sizeof(desc));
			m_Texture->TrackDescriptor(m_DescHash, weak_from_this());
			return true;
		}

		return false;
	}

	DepthStencilView::DepthStencilView()
		: ResourceDescriptor(RESOURCE_DESCRIPTOR_TYPE_DEPTH_STENCIL_VIEW)
	{
	}

	bool DepthStencilView::Init(const DepthStencilViewDesc& desc, const Ref<Texture>& resource)
	{
		if (AllocateDescriptor())
		{
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = D3DConvertFormat(resource->m_TextureDesc.m_Format);
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			GetDevice().GetD3DDevice()->CreateDepthStencilView(resource->GetD3DResource(), &dsvDesc, m_D3DHandle);

			m_Texture = resource;
			m_DescHash = hash_fnv64(reinterpret_cast<const uint8_t*>(&desc), sizeof(desc));
			m_Texture->TrackDescriptor(m_DescHash, weak_from_this());
			return true;
		}

		return false;
	}

	BufferView::BufferView()
		: ResourceDescriptor(RESOURCE_DESCRIPTOR_TYPE_BUFFER_VIEW)
	{
	}

	bool BufferView::Init(const BufferViewDesc& desc, const Ref<Buffer>& resource)
	{
		if (AllocateDescriptor())
		{
			if (desc.m_Writable)
			{
				// TODO: add support for all buffer types (typed, structured, byte buffers etc.)

				D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
				uavDesc.Format = DXGI_FORMAT_UNKNOWN;
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				uavDesc.Buffer.FirstElement = desc.m_First;
				uavDesc.Buffer.NumElements = desc.m_Last - desc.m_First;
				uavDesc.Buffer.StructureByteStride = desc.m_ElementSize;
				uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
				GetDevice().GetD3DDevice()->CreateUnorderedAccessView(resource->GetD3DResource(), nullptr, &uavDesc, m_D3DHandle);
			}
			else
			{
				// TODO: add support for all buffer types (typed, structured, byte buffers etc.)

				D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.FirstElement = desc.m_First;
				srvDesc.Buffer.NumElements = desc.m_Last - desc.m_First;
				srvDesc.Buffer.StructureByteStride = desc.m_ElementSize;
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				GetDevice().GetD3DDevice()->CreateShaderResourceView(resource->GetD3DResource(), &srvDesc, m_D3DHandle);
			}

			m_Buffer = resource;
			m_DescHash = hash_fnv64(reinterpret_cast<const uint8_t*>(&desc), sizeof(desc));
			m_Buffer->TrackDescriptor(m_DescHash, weak_from_this());
			return true;
		}

		return false;
	}
}