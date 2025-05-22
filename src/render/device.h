#pragma once

#include "rendercommon.h"
#include "context.h"
#include "swapchain.h"
#include "pipelinestate.h"
#include "texture.h"
#include "buffer.h"

namespace vkr::Render
{
	class DescriptorHeap;
	class RootSignature;
	class ShaderCompiler;
	
	class Device
	{
	public:
		Device();
		~Device();

		bool Init(bool enableDebugLayer = false);

		// Create render resources (textures, buffers, PSOs...)
		Context* CreateContext();
		
		SwapChain* CreateSwapChain(void* windowHandle, const Vector2u& size);

		Shader* CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_6);
		PipelineState* CreatePipelineState(const PipelineStateDesc& desc);

		Texture* CreateTexture(const TextureDesc& desc);
		ResourceDescriptor* GetOrCreateDescriptor(Texture* tex, const ResourceDescriptorDesc& desc);
		Buffer* CreateBuffer(const BufferDesc& desc);

	private:
		void InitRootSignatures();
		void InitDescriptorHeaps();

	private:
		ComPtr<IDXGIFactory2> m_Factory;
		ComPtr<IDXGIAdapter1> m_Adapter;
		ComPtr<ID3D12Device> m_Device;

		ComPtr<ID3D12CommandQueue> m_CommandQueue;

		ShaderCompiler* m_ShaderCompiler;
		RootSignature* m_RootSignatures[PIPELINE_STATE_TYPE_COUNT];

		DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	};
}

