#pragma once

#include "rendercommon.h"

namespace vkr::Render
{
	class Resource; 
	class Texture;
	class Buffer;

	enum ResourceDescriptorType : uint8_t
	{
		RESOURCE_DESCRIPTOR_TYPE_TEXTURE_VIEW,
		RESOURCE_DESCRIPTOR_TYPE_BUFFER_VIEW,
		RESOURCE_DESCRIPTOR_TYPE_SAMPLER,
		RESOURCE_DESCRIPTOR_TYPE_RENDER_TARGET_VIEW,
		RESOURCE_DESCRIPTOR_TYPE_DEPTH_STENCIL_VIEW,

		RESOURCE_DESCRIPTOR_TYPE_COUNT
	};

	class ResourceDescriptor
	{
		friend class DescriptorHeap;
	public:
		static constexpr uint32_t g_InvalidIndex = uint32_t(-1);

	public:
		ResourceDescriptor(ResourceDescriptorType type);
		~ResourceDescriptor();

		uint32_t GetIndex() const { return m_DescriptorIndex; }
		D3D12_CPU_DESCRIPTOR_HANDLE& GetHandle() { return m_D3DHandle; }

	protected:
		bool AllocateDescriptor();
		bool FreeDescriptor();

	protected:
		D3D12_CPU_DESCRIPTOR_HANDLE m_D3DHandle;
		uint32_t m_DescriptorIndex;

		const ResourceDescriptorType m_Type;
	};


	/////////////////////////////////////////////////////////////
	// TEXTURE VIEW
	struct TextureViewDesc
	{
		uint32_t m_Mip = 0; //Use -1 to reference all mips when applicable
		bool m_Writable = false;
	};

	class TextureView : public ResourceDescriptor
	{
	public: 
		TextureView();
		bool Init(const TextureViewDesc& desc, const Ref<Texture>& resource);

		Texture* GetTexture() const { return m_Texture.get(); }
	private:
		Ref<Texture> m_Texture;
	};
	/////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////
	// RENDER TARGET VIEW
	struct RenderTargetViewDesc
	{
		uint32_t m_Mip = 0; //Use -1 to reference all mips when applicable
	};

	class RenderTargetView : public ResourceDescriptor
	{
	public:
		RenderTargetView();
		bool Init(const RenderTargetViewDesc& desc, const Ref<Texture>& resource);

		Texture* GetTexture() const { return m_Texture.get(); }
	private:
		Ref<Texture> m_Texture;
	};
	/////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////
	// DEPTH STENCIL VIEW

	struct DepthStencilViewDesc
	{
		uint32_t m_Mip = 0; //Use -1 to reference all mips when applicable
	};

	class DepthStencilView : public ResourceDescriptor
	{
	public:
		DepthStencilView();
		bool Init(const DepthStencilViewDesc& desc, const Ref<Texture>& resource);

		Texture* GetTexture() const { return m_Texture.get(); }
	private:
		Ref<Texture> m_Texture;
	};
	/////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////
	// BUFFER VIEW
	struct BufferViewDesc
	{
		uint32_t m_First = 0;
		uint32_t m_Last = 0;
		uint32_t m_ElementSize = 0;
		bool m_Writable = false;

	};

	class BufferView : public ResourceDescriptor
	{
	public:
		BufferView();
		bool Init(const BufferViewDesc& desc, const Ref<Buffer>& resource);

		Buffer* GetBuffer() const { return m_Buffer.get(); }
	private:
		Ref<Buffer> m_Buffer;
	};
	/////////////////////////////////////////////////////////////
}