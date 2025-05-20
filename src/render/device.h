#pragma once

#include "render/rendercommon.h"
#include "render/context.h"
#include "render/swapchain.h"
#include "render/pipelinestate.h"
#include "render/texture.h"
#include "render/buffer.h"

namespace vkr::Render
{
	class ShaderCompiler;
	class RootSignature;
	class TextureLoader;
	class Device
	{
	public:
		Device();
		~Device();

		bool Init();

		Context* CreateContext(ContextType contextType);
		
		SwapChain* CreateSwapChain(void* windowHandle, const Vector2u& size);

		Shader* CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_6);
		PipelineState* CreatePipelineState(const PipelineStateDesc& desc);

		Texture* CreateTexture(const TextureDesc& desc, const TextureData* initialData = nullptr);
		Texture* LoadTexture(const std::filesystem::path& filepath);

		Buffer* CreateBuffer(const BufferDesc& desc);

		ID3D12Device* GetD3DDevice() const;
		IDXGIFactory2* GetDXGIFactory() const;
		IDXGIAdapter1* GetDXGIAdapter() const;
		ID3D12CommandQueue* GetCommandQueue(ContextType contextType) const;

	private:
		void InitRootSignatures();
		void InitTextureLoaders();
		void InitCommandQueues();

	private:
		ComPtr<IDXGIFactory2> m_Factory;
		ComPtr<IDXGIAdapter1> m_Adapter;
		ComPtr<ID3D12Device> m_Device;

		ComPtr<ID3D12CommandQueue> m_CommandQueue[CONTEXT_TYPE_COUNT];
		ComPtr<ID3D12Fence> m_CommandQueueFence[CONTEXT_TYPE_COUNT];
		uint64_t m_CommandQueueFenceValue[CONTEXT_TYPE_COUNT];

		ShaderCompiler* m_ShaderCompiler;
		RootSignature* m_RootSignatures[PIPELINE_STATE_TYPE_COUNT];

		std::unordered_map<std::filesystem::path, TextureLoader*> m_TextureLoaderByExtension;
	};
}

