#pragma once
#include "rendercommon.h"
#include "resource.h"

namespace vkr::Render
{
	struct TextureDesc
	{
		Vector3u m_Size = { 1, 1, 1 };
		uint16_t m_ArraySize = 1;
		Format m_Format = FORMAT_UNKNOWN;
		ResourceDimension m_Dimension = ResourceDimension::Texture2D;
		bool m_Writable = false;
		bool m_AllowRenderTarget = false;
		bool m_AllowDepthStencil = false;
		bool m_CalculateMips = false;
	};

	struct TextureData
	{
		std::vector<uint8_t> m_Data;
		uint32_t m_ByteSize;
	};

	class Texture : public Resource
	{
	public:
		Texture();
		~Texture();

		bool Init(const TextureDesc& desc, const TextureData* initialData = nullptr);
		bool InitWithResource(const TextureDesc& desc, const ComPtr<ID3D12Resource>& resource, const ResourceStateTracking& initialState);

		void UploadData(const TextureData& data);

		TextureDesc m_TextureDesc;
	};
}