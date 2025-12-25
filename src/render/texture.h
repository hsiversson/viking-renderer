#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct TextureDesc
	{
		Vector3u m_Size = { 1, 1, 1 };
		uint16_t m_ArraySize = 1;
		uint16_t m_MipLevels = UINT16_MAX;
		Format m_Format = FORMAT_UNKNOWN;
		ResourceDimension m_Dimension = ResourceDimension::Texture2D;
		bool m_Writable = false;
		bool m_AllowRenderTarget = false;
		bool m_AllowDepthStencil = false;
		Vector4f m_ClearValue = { 0,0,0,0 };
		uint32_t m_NumSamples = 1;

		CpuAccess m_CpuAccess = CPU_ACCESS_NONE;
		GpuAccess m_GpuAccess = GPU_ACCESS_READ;

		const char* m_Name = nullptr;
	};

	struct TextureData
	{
		struct Subresource
		{
			std::vector<uint8_t> m_Data;
			uint64_t m_RowPitch;
			uint64_t m_SlicePitch;
		};
		std::vector<Subresource> m_Subresources;
	};

	class Texture : public Resource
	{
	public:
		struct Subresource
		{
			uint32_t m_Width;
			uint32_t m_Height;
			uint32_t m_Depth;
			uint32_t m_RowPitch;
			Format m_Format;
		};
		struct PlacedSubresource
		{
			uint64_t m_Offset;
			Subresource m_Subresource;
		};

	public:
		Texture();
		~Texture();

		bool Init(const TextureDesc& desc, const TextureData* initialData = nullptr);
		bool InitWithResource(const TextureDesc& desc, const ComPtr<ID3D12Resource>& resource, const ResourceStateTracking& initialState);

		void UploadData(const TextureData& data);
		void DownloadData(uint32_t size, void* dst);

		void GetSubresourceFootprints(uint32_t firstSubresource, uint32_t numSubresources, PlacedSubresource* subresources, uint32_t* numRows = nullptr, uint64_t* rowByteSize = nullptr, uint64_t* totalByteSize = nullptr) const;

		TextureDesc m_TextureDesc;
	};

	class TextureCache
	{
	public:
		Ref<Texture> Get(const std::filesystem::path& filepath) const;
		void Insert(const std::filesystem::path& filepath, const Ref<Texture>& texture);

	private:
		std::unordered_map<std::filesystem::path, Ref<Texture>> m_Cache;
	};
}