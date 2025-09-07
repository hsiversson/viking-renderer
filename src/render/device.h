#pragma once

#include "render/rendercommon.h"
#include "render/context.h"
#include "render/swapchain.h"
#include "render/pipelinestate.h"
#include "render/texture.h"
#include "render/buffer.h"
#include "render/descriptorheap.h"
#include "render/renderthread.h"

namespace vkr::Render
{
	class CommandListPool;
	class CommandQueue;
	class DescriptorHeap;
	class NvStreamline;
	class Profiler;
	class RenderResourceDestructionQueue;
	class RootSignature;
	class ShaderCompiler;
	class TextureLoader;

	class Device
	{
		friend Device* GetDevice();
	public:
		Device();
		~Device();

		bool Init();

		void BeginFrame(uint64_t frameIndex);
		void EndFrame();

		void WaitForGpuIdle();
		
		Ref<SwapChain> CreateSwapChain(void* windowHandle, const Vector2u& size);

		Ref<Shader> CreateShader(const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_7);
		Ref<Shader> CreateShaderFromString(const std::string& sourceCode, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_7);
		Ref<Shader> CreateShaderFromString(const std::wstring& sourceCode, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_7);
		Ref<PipelineState> CreatePipelineState(const PipelineStateDesc& desc);

		Ref<Texture> CreateTexture(const TextureDesc& desc, const TextureData* initialData = nullptr);
		Ref<Texture> LoadTexture(const std::filesystem::path& filepath);
		Ref<TextureView> CreateTextureView(const TextureViewDesc& desc, const Ref<Texture>& resource);
		Ref<RenderTargetView> CreateRenderTargetView(const RenderTargetViewDesc& desc, const Ref<Texture>& resource);
		Ref<DepthStencilView> CreateDepthStencilView(const DepthStencilViewDesc& desc, const Ref<Texture>& resource);

		Ref<Buffer> CreateBuffer(const BufferDesc& desc, uint32_t initialDataSize = 0, const void* initialData = nullptr);
		Ref<BufferView> CreateBufferView(const BufferViewDesc& desc, const Ref<Buffer>& resource);

		TempBuffer GetTempBuffer(TempBufferUsage usage, uint32_t byteSize, uint32_t initialDataSize = 0, const void* initialData = nullptr); // TempBuffers only last until the end of the frame, then their memory is reused

		Ref<Buffer> CreateTLAS(uint32_t numRtInstanceDescs, RaytracingInstanceDesc* rtInstanceDescs);
		Ref<Buffer> CreateBLAS(uint32_t numRtGeometryDescs, RaytracingGeometryDesc* rtGeometryDescs);

		void SetCurrentSwapChain(const Ref<SwapChain>& swapChain);
		const Ref<SwapChain>& GetCurrentSwapChain() const;

		ID3D12Device* GetD3DDevice() const;
		ID3D12Device10* GetD3DDevice10() const;
		IDXGIFactory2* GetDXGIFactory() const;
		IDXGIAdapter1* GetDXGIAdapter() const;
		const Ref<CommandQueue>& GetCommandQueue(ContextType contextType) const;
		const Ref<CommandListPool>& GetCommandListPool(ContextType contextType) const;
		Ref<Context> GetContext(ContextType contextType) const;
		RenderThread* GetRenderThread(ContextType contextType) const;
		DescriptorHeap* GetDescriptorHeap(DescriptorHeapType type) const;
		NvStreamline* GetNvStreamline();

#if ENABLE_PROFILING
		Profiler* GetProfiler() const;
#endif

		void FlushDeferredDestructionQueue();

	private:
		void InitNvStreamline();
		void InitRootSignatures();
		void InitTextureLoaders();
		void InitCommandQueues();
		void InitDescriptorHeaps();

	private:
		ComPtr<IDXGIFactory2> m_Factory;
		ComPtr<IDXGIAdapter1> m_Adapter;
		ComPtr<ID3D12Device> m_Device;
		ComPtr<ID3D12Device10> m_Device10;

		Ref<Context> m_Contexts[CONTEXT_TYPE_COUNT];//For now lets keep just a single context of every type on the device itself (prone to change)
		Ref<CommandQueue> m_CommandQueue[CONTEXT_TYPE_COUNT];
		Ref<CommandListPool> m_CommandListPool[CONTEXT_TYPE_COUNT];
		UniquePtr<RenderThread> m_RenderThreads[CONTEXT_TYPE_COUNT];

		std::mutex m_RaytracingBuildQueueMutex;
		Ref<Context> m_RaytracingBuildContext;
		Ref<CommandQueue> m_RaytracingBuildQueue;

		UniquePtr<ShaderCompiler> m_ShaderCompiler;
		Ref<RootSignature> m_RootSignatures[PIPELINE_STATE_TYPE_COUNT];

		UniquePtr<RenderResourceDestructionQueue> m_RenderResourceDestructionQueue;

		std::unordered_map<std::filesystem::path, UniquePtr<TextureLoader>> m_TextureLoaderByExtension;
		UniquePtr<DescriptorHeap> m_DescriptorHeaps[RESOURCE_DESCRIPTOR_TYPE_COUNT];

		// Temp buffers
		UniquePtr<TempBufferAllocator> m_TempBufferAllocators[TEMP_BUFFER_USAGE_COUNT];
		std::vector<UniquePtr<TempBufferAllocator>> m_TempBuffersPendingDelete;

		Ref<SwapChain> m_CurrentSwapChain;

		ShaderCache m_ShaderCache;
		TextureCache m_TextureCache;

		UniquePtr<NvStreamline> m_NvStreamline;

#if ENABLE_PROFILING
		UniquePtr<Profiler> m_Profiler;
#endif

		static Device* g_Instance;
	};

	inline Device* GetDevice() { return Device::g_Instance; }
	Ref<RenderTaskEvent> QueueRenderTask(ContextType type, RenderTaskFn task, RenderTaskFlags flags = RENDER_TASK_FLAG_NONE);
	Ref<RenderTaskEvent> QueueGraphicsTask(RenderTaskFn task, RenderTaskFlags flags = RENDER_TASK_FLAG_NONE);
	Ref<RenderTaskEvent> QueueComputeTask(RenderTaskFn task, RenderTaskFlags flags = RENDER_TASK_FLAG_NONE);
	Ref<RenderTaskEvent> QueueCopyTask(RenderTaskFn task, RenderTaskFlags flags = RENDER_TASK_FLAG_NONE);
}

