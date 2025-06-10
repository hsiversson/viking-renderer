#pragma once

#include "render/rendercommon.h"
#include "render/context.h"
#include "render/swapchain.h"
#include "render/pipelinestate.h"
#include "render/texture.h"
#include "render/buffer.h"

namespace vkr::Render
{
	class DescriptorHeap;
	class RootSignature;
	class ShaderCompiler;
	class RootSignature;
	class TextureLoader;
	class CommandQueue;
	class CommandListPool;

	struct RtInstanceDesc
	{
		Mat43 m_Transform;
		uint32_t m_InstanceId;
		Ref<Buffer> m_BLAS;
	};

	struct RtGeometryDesc
	{
		Ref<Buffer> m_VertexBuffer; // positions only?
		Ref<Buffer> m_IndexBuffer;
	};

	class Device
	{
		friend Device& GetDevice();
	public:
		Device();
		~Device();

		bool Init();

		Ref<Context> CreateContext(ContextType contextType);
		
		Ref<SwapChain> CreateSwapChain(void* windowHandle, const Vector2u& size);

		Ref<Shader> CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_6);
		Ref<PipelineState> CreatePipelineState(const PipelineStateDesc& desc);

		Ref<Texture> CreateTexture(const TextureDesc& desc, const TextureData* initialData = nullptr);
		Ref<Texture> LoadTexture(const std::filesystem::path& filepath);
		Ref<ResourceDescriptor> GetOrCreateDescriptor(Texture* tex, const ResourceDescriptorDesc& desc);
		Ref<Buffer> CreateBuffer(const BufferDesc& desc);
		Ref<ResourceDescriptor> GetOrCreateDescriptor(Buffer* buf, const ResourceDescriptorDesc& desc);

		TempBuffer GetTempBuffer(uint32_t byteSize, uint32_t initialDataSize = 0, const void* initialData = nullptr); // TempBuffers only last until the end of the frame, then their memory is reused

		Ref<Buffer> CreateTLAS(uint32_t numRtInstanceDescs, RtInstanceDesc* rtInstanceDescs);
		Ref<Buffer> CreateBLAS(uint32_t numRtGeometryDescs, RtGeometryDesc* rtGeometryDescs);

		ID3D12Device* GetD3DDevice() const;
		IDXGIFactory2* GetDXGIFactory() const;
		IDXGIAdapter1* GetDXGIAdapter() const;
		const Ref<CommandQueue>& GetCommandQueue(ContextType contextType) const;
		const Ref<CommandListPool>& GetCommandListPool(ContextType contextType) const;
		Ref<Context> GetContext(ContextType contextType) const;

	private:
		void InitRootSignatures();
		void InitTextureLoaders();
		void InitCommandQueues();
		void InitDescriptorHeaps();

		Ref<Buffer> CreateRaytracingAccelerationStructure(D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& buildDesc);

	private:
		ComPtr<IDXGIFactory2> m_Factory;
		ComPtr<IDXGIAdapter1> m_Adapter;
		ComPtr<ID3D12Device> m_Device;
		ComPtr<ID3D12Device5> m_Device5;

		Ref<Context> m_Contexts[CONTEXT_TYPE_COUNT];//For now lets keep just a single context of every type on the device itself (prone to change)
		Ref<CommandQueue> m_CommandQueue[CONTEXT_TYPE_COUNT];
		Ref<CommandListPool> m_CommandListPool[CONTEXT_TYPE_COUNT];

		Ref<CommandQueue> m_RaytracingBuildQueue;
		Ref<CommandListPool> m_RaytracingBuildPool;

		UniquePtr<ShaderCompiler> m_ShaderCompiler;
		Ref<RootSignature> m_RootSignatures[PIPELINE_STATE_TYPE_COUNT];

		std::unordered_map<std::filesystem::path, UniquePtr<TextureLoader>> m_TextureLoaderByExtension;
		DescriptorHeap* m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

		static Device* g_Instance;
	};

	inline Device& GetDevice() { return *Device::g_Instance; }
}

