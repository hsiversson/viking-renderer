#pragma once
#include "render/rendercommon.h"
#include "render/rendertaskevent.h"

namespace vkr::Render
{
	class Texture;
	class Buffer;
	class DepthStencilView;
	class PipelineState;
	class RenderTargetView;
	class ResourceDescriptor;
	class RootSignature;
	class CommandList;
	class CommandQueue;

	enum ContextType : uint8_t
	{
		CONTEXT_TYPE_GRAPHICS,
		CONTEXT_TYPE_PRESENT = CONTEXT_TYPE_GRAPHICS,
		CONTEXT_TYPE_COMPUTE,
		CONTEXT_TYPE_COPY,

		CONTEXT_TYPE_COUNT
	};

	struct TextureBarrierDesc
	{
		ResourceStateAccess m_TargetAccess;
		ResourceStateSync m_TargetSync;
		ResourceStateLayout m_TargetLayout;
		Texture* m_Texture;
		// subresources?
	};

	struct BufferBarrierDesc
	{
		ResourceStateAccess m_TargetAccess;
		ResourceStateSync m_TargetSync;
		Buffer* m_Buffer;
	};

	struct GlobalBarrierDesc
	{
		ResourceStateAccess m_SourceAccess;
		ResourceStateAccess m_TargetAccess;
		ResourceStateSync m_SourceSync;
		ResourceStateSync m_TargetSync;
	};

	struct RaytracingInstanceDesc
	{
		Mat44 m_Transform;
		uint32_t m_InstanceId;
		Ref<Buffer> m_BLAS;
	};

	struct RaytracingGeometryDesc
	{
		Ref<Buffer> m_VertexBuffer; // positions only?
		Ref<Buffer> m_IndexBuffer;
	};

	struct RaytracingAccelerationStructureBuildDesc
	{
		enum class Type
		{
			TopLevel,
			BottomLevel,
		};

		Type m_Type;
		std::vector<RaytracingInstanceDesc> m_InstanceDescs;
		std::vector<RaytracingGeometryDesc> m_GeometryDescs;
	};

	class Context
	{
	public:
		Context(ContextType type, const Ref<CommandQueue>& commandQueue);
		~Context();

		void Begin();
		void End();
		Fence Flush();

		// Markers
		void SetMarker(const char* label);
		void EndMarker();

		// Compute
		void Dispatch(const Vector3u& Groups);
		void DispatchThreads(const Vector3u& threads);
		void DispatchThreads(Ref<PipelineState> pipelineState, const Vector3u& threads);

		//Draw
		void Draw(uint32_t vertexCount, uint32_t startVertex = 0);
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0, uint32_t startInstance = 0);

		void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t startVertex = 0);
		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0, uint32_t startVertex = 0, uint32_t startInstance = 0);

		//Render state
		void BindPSO(Ref<PipelineState> pipelineState);
		void BindRootConstantBuffers(Ref<Buffer>* buffers, size_t bufferCount, uint64_t* offsets = nullptr);
		void BindVertexBuffers(Ref<Buffer>* vertexbuffers, size_t vertexbuffercount);
		void BindIndexBuffer(Ref<Buffer> indexbuffer);
		void BindRenderTargets(Ref<RenderTargetView>* rtviews, size_t viewCount);
		void BindDepthStencil(Ref<DepthStencilView> dsview);
		void SetPrimitiveTopology(PrimitiveTopology topologyType);
		void SetViewport(uint32_t offsetX, uint32_t offsetY, uint32_t width, uint32_t height, float depthMin = 0.0f, float depthMax = 1.0f);
		void SetScissorRect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom);

		//Barrier
		void TextureBarrier(uint32_t numBarriers, const TextureBarrierDesc* barrierDescs);
		void TextureBarrier(const TextureBarrierDesc& barrierDesc);
		void BufferBarrier(uint32_t numBarriers, const BufferBarrierDesc* barrierDescs);
		void BufferBarrier(const BufferBarrierDesc& barrierDesc);
		void GlobalBarrier(uint32_t numBarriers, const GlobalBarrierDesc* barrierDescs);
		void GlobalBarrier(const GlobalBarrierDesc& barrierDesc);

		// Clear
		void ClearRenderTargets(Ref<RenderTargetView>* rtvs, size_t numRtvs);
		void ClearDepthStencil(Ref<DepthStencilView> dsv, float clearValue);

		// Raytracing acceleration structure
		Ref<Buffer> BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructureBuildDesc& desc);

		// Copy
		void CopyResource(Buffer* dst, Buffer* src);
		void CopyResource(Texture* dst, Texture* src);
		void CopyBuffer(Buffer* dst, uint64_t dstOffset, Buffer* src, uint64_t srcOffset, uint32_t size);
		void CopyTexture(Texture* dst, Texture* src);

		// Synchronization
		void InsertWait(const Fence& fence);

		ContextType GetType() const;
		CommandList* GetCommandList() const;

		static Context* GetCurrentContext();

	private:
		struct DrawState
		{
			PrimitiveTopology m_Topology = PRIMITIVE_TOPOLOGY_UNDEFINED;
			std::vector<Ref<Buffer>> m_VertexBuffers;
			Ref<Buffer> m_IndexBuffer;
			RootSignature* m_RootSignature = nullptr;
			Ref<PipelineState> m_PipelineState = nullptr;
			std::vector<Ref<Buffer>> m_RootCB;
			std::vector<uint64_t> m_RootCBOffsets;
			std::vector<Ref<RenderTargetView>> m_RenderTargets;
			Ref<DepthStencilView> m_DepthStencil;
		};

		void UpdateState();

		Ref<CommandQueue> m_CommandQueue;

		Ref<CommandList> m_CommandList;
		ID3D12GraphicsCommandList* m_CurrentD3DCommandList;
		ID3D12GraphicsCommandList7* m_CurrentD3DCommandList7;
		uint32_t m_NumRecordedCommands;

		std::vector<Fence> m_FencesToWaitFor;
		std::vector<Ref<CommandList>> m_CommandListsToSubmit;
		Fence m_LastFlushEvent;

		DrawState CurrentState;
		DrawState NewState;
		bool m_StateUpdate = false;
		bool m_RenderTargetUpdate = false;

		const ContextType m_Type;
		static thread_local Context* g_CurrentContext;
	};
}