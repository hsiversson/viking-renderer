#include "texture.h"

#include "device.h"
#include "d3dconvert.h"
#include "commandlist.h"

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

		D3D12_RESOURCE_DESC1 textureDesc = D3DConvertTextureDesc(desc);
		m_TextureDesc = desc;
		m_TextureDesc.m_MipLevels = textureDesc.MipLevels;

		D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_COMMON;
		m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_COMMON;
		m_StateTracking.m_CurrentLayout = RESOURCE_STATE_LAYOUT_COMMON;
		m_StateTracking.m_CurrentSync = RESOURCE_STATE_SYNC_ALL;
		if (desc.m_Writable)
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
			m_StateTracking.m_CurrentAccess = RESOURCE_STATE_ACCESS_DEPTH_STENCIL_WRITE;
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

		const D3D12_HEAP_PROPERTIES heapProps = D3DGetDefaultHeapProperties();
		HRESULT hr = GetDevice()->GetD3DDevice10()->CreateCommittedResource3(&heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, initialLayout, (desc.m_AllowRenderTarget || desc.m_AllowDepthStencil) ? &optimizedClearValue : nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&m_Resource));
		if (FAILED(hr))
		{
			return false;
		}

		if (initialData)
		{
			UploadData(*initialData);
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

	void Texture::UploadData(const TextureData& data)
	{
		if (true /*isRenderThread*/)
		{
			// Get staging buffer and place texture data
			// copy operation on context from staging to persistent texture (m_Resource)
			// barriers?

			Device* device = GetDevice();
			ID3D12Device10* d3dDevice = device->GetD3DDevice10();
			Ref<Context> ctx = device->GetContext(CONTEXT_TYPE_GRAPHICS);
			ctx->Begin();
			ID3D12GraphicsCommandList* d3dCmdList = ctx->GetCommandList()->GetD3DCommandList();

			const D3D12_RESOURCE_DESC1 tempDesc = D3DConvertTextureDesc(m_TextureDesc);
			const uint32_t blockSize = GetFormatBlockSize(m_TextureDesc.m_Format);
			const uint32_t numSubresources = m_TextureDesc.m_MipLevels * m_TextureDesc.m_ArraySize;
			assert(data.m_Subresources.size() == numSubresources);
			
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(numSubresources);
			std::vector<uint64_t> rowSizes(numSubresources);
			std::vector<uint32_t> numRows(numSubresources);
			uint64_t totalTempBufferSize = 0;
			d3dDevice->GetCopyableFootprints1(&tempDesc, 0, numSubresources, 0, footprints.data(), numRows.data(), rowSizes.data(), &totalTempBufferSize);

			TempBuffer tempBuffer = device->GetTempBuffer(totalTempBufferSize);
			for (uint32_t arrayIdx = 0; arrayIdx < m_TextureDesc.m_ArraySize; ++arrayIdx)
			{
				for (uint32_t mipIdx = 0; mipIdx < m_TextureDesc.m_MipLevels; ++mipIdx)
				{
					const uint32_t subIdx = mipIdx + (arrayIdx * m_TextureDesc.m_MipLevels);
					const TextureData::Subresource& subresource = data.m_Subresources[subIdx];
					const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& footprint = footprints[subIdx];

					uint8_t* dstBase = tempBuffer.m_Buffer->GetDataPtr() + footprint.Offset;
					const uint8_t* srcBase = subresource.m_Data.data();

					const uint64_t srcRowPitch = subresource.m_RowPitch ? subresource.m_RowPitch : rowSizes[subIdx];
					const uint64_t dstRowPitch = footprint.Footprint.RowPitch;

					const uint64_t srcSlicePitch = subresource.m_SlicePitch ? subresource.m_SlicePitch : srcRowPitch * numRows[subIdx];
					const uint64_t dstSlicePitch = dstRowPitch * numRows[subIdx];

					for (uint32_t z = 0; z < footprint.Footprint.Depth; ++z)
					{
						const uint8_t* srcSlice = srcBase + z * srcSlicePitch;
						uint8_t*	   dstSlice = dstBase + z * dstSlicePitch;

						for (uint32_t y = 0; y < numRows[subIdx]; ++y)
						{
							const uint8_t* srcRow = srcSlice + y * srcRowPitch;
							uint8_t* dstRow = dstSlice + y * dstRowPitch;
							memcpy(dstRow, srcRow, srcRowPitch);
						}
					}
				}
			}

			D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
			dstLoc.pResource = m_Resource.Get();
			dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

			D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
			srcLoc.pResource = tempBuffer.m_Buffer->GetD3DResource();
			srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			srcLoc.PlacedFootprint.Offset = 0;

			D3D12_BOX srcBox = { 0,0,0 };
			for (uint32_t arrayIdx = 0; arrayIdx < m_TextureDesc.m_ArraySize; ++arrayIdx)
			{
				for (uint32_t mipIdx = 0; mipIdx < m_TextureDesc.m_MipLevels; ++mipIdx)
				{
					const uint32_t subIdx = mipIdx + (arrayIdx * m_TextureDesc.m_MipLevels);

					srcLoc.PlacedFootprint = footprints[subIdx];
					dstLoc.SubresourceIndex = subIdx;

					// mip‑sizes shrink per level
					const uint32_t mipWidth = std::max(blockSize, m_TextureDesc.m_Size.x >> mipIdx);
					const uint32_t mipHeight = std::max(blockSize, m_TextureDesc.m_Size.y >> mipIdx);
					const uint32_t mipDepth = std::max(1u, m_TextureDesc.m_Size.z >> mipIdx);

					srcBox.right = mipWidth;
					srcBox.bottom = mipHeight;
					srcBox.back = mipDepth;

					d3dCmdList->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, &srcBox);
				}
			}

			ctx->End();
			ctx->Flush();
		}
		//else
		//{
		//	// Launch copy task on initialization context thread, wait for event?
		//}
	}

}