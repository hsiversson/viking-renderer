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

		void ClearStateCache();

		// Markers
		void BeginMarker(const char* label, uint32_t color);
		void EndMarker();

		// Compute
		void Dispatch(const Vector3u& Groups);
		void DispatchThreads(const Vector3u& threads);
		void DispatchThreads(PipelineState* pipelineState, const Vector3u& threads);

		//Draw
		void Draw(uint32_t vertexCount, uint32_t startVertex = 0);
		void DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0, uint32_t startInstance = 0);

		void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0, uint32_t startVertex = 0);
		void DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0, uint32_t startVertex = 0, uint32_t startInstance = 0);

		//Render state
		void BindPipelineState(PipelineState* pipelineState);
		void BindLocalConstantBuffer(Buffer* buffer, uint64_t offset, uint32_t slot);
		void BindLocalConstantBuffer(uint32_t byteSize, const void* data, uint32_t slot);
		void BindGlobalConstantBuffer(Buffer* buffer, uint64_t offset, GlobalConstantBufferSlot slot);
		void BindGlobalConstantBuffer(uint32_t byteSize, const void* data, GlobalConstantBufferSlot slot);
		void BindVertexBuffers(uint32_t numVertexBuffers, Buffer** vertexBuffers, const uint64_t* offsets = nullptr, const uint32_t* sizes = nullptr, const uint32_t* strides = nullptr);
		void BindVertexBuffer(Buffer* vertexBuffers, const uint64_t offsets = 0, const uint32_t size = 0, const uint32_t stride = 0);
		void BindIndexBuffer(Buffer* indexbuffer, const uint64_t offset = 0, const uint32_t size = 0, const Format format = FORMAT_UNKNOWN);
		void BindRenderTargets(uint32_t numRenderTargets, RenderTargetView** renderTargetViews);
		void BindDepthStencil(DepthStencilView* depthStencilView);
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
		void ClearRenderTargets(uint32_t numRenderTargets, RenderTargetView** renderTargetViews, const Vector4f* clearValues = nullptr);
		void ClearRenderTarget(RenderTargetView* renderTargetView, const Vector4f& clearValue = Vector4f(0.0f));
		void ClearDepthStencil(DepthStencilView* dsv, float clearValue);

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

		const Fence& GetLastFence() const;

		static Context* GetCurrentContext();

	private:
		struct RenderStateCache
		{
			void Clear();

			PrimitiveTopology m_Topology = PRIMITIVE_TOPOLOGY_UNDEFINED;

			std::vector<Buffer*> m_VertexBuffers;
			std::vector<uint64_t> m_VertexBufferOffsets;
			std::vector<uint32_t> m_VertexBufferSizes;
			std::vector<uint32_t> m_VertexBufferStrides;

			Buffer* m_IndexBuffer;
			uint64_t m_IndexBufferOffset = 0;
			uint32_t m_IndexBufferSize = 0;
			Format m_IndexBufferFormat = FORMAT_UNKNOWN;

			RootSignature* m_RootSignature = nullptr;
			PipelineState* m_PipelineState = nullptr;
			
			std::array<Buffer*, MAX_NUM_LOCAL_CONSTANT_BUFFERS> m_LocalConstantBuffers;
			std::array<uint64_t, MAX_NUM_LOCAL_CONSTANT_BUFFERS> m_LocalConstantBufferOffsets;
			std::array<bool, MAX_NUM_LOCAL_CONSTANT_BUFFERS> m_LocalConstantsDirty;

			std::array<Buffer*, GLOBAL_CONSTANT_BUFFER_COUNT> m_GlobalConstantBuffers;
			std::array<uint64_t, GLOBAL_CONSTANT_BUFFER_COUNT> m_GlobalConstantBufferOffsets;
			std::array<bool, GLOBAL_CONSTANT_BUFFER_COUNT> m_GlobalConstantsDirty;

			std::array<RenderTargetView*, MAX_NUM_RENDER_TARGETS> m_RenderTargets;
			DepthStencilView* m_DepthStencil = nullptr;

			uint64_t m_TopologyDirty : 1;
			uint64_t m_VertexBuffersDirty : 1;
			uint64_t m_IndexBufferDirty : 1;
			uint64_t m_RootSignatureDirty : 1;
			uint64_t m_PipelineStateDirty : 1;
			uint64_t m_RenderTargetsDirty : 1;
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

		RenderStateCache m_StateCache;

		const ContextType m_Type;
	};

	class ContextMarkerScope
	{
	public:
		ContextMarkerScope(Context* ctx, const char* label, uint32_t color) : m_Ctx(ctx) { ctx->BeginMarker(label, color); }
		~ContextMarkerScope() { m_Ctx->EndMarker(); }
	private:
		Context* m_Ctx;
	};

#define _SET_CTX_MARKER_CONCAT_IMPL(x, y) x##y
#define _SET_CTX_MARKER_CONCAT(x, y) _SET_CTX_MARKER_CONCAT_IMPL(x, y)

// TODO: Begin/End category? Which would make any sub-scope markers use the same color as the previous scope.

#define SET_CONTEXT_MARKER(ctx, label) vkr::Render::ContextMarkerScope _SET_CTX_MARKER_CONCAT(_ctxMarkerScope_, __LINE__)(ctx, label, vkr::Random::Rnd_u32())
#define SET_CONTEXT_MARKER_COLORED(ctx, label, color) vkr::Render::ContextMarkerScope _SET_CTX_MARKER_CONCAT(_ctxMarkerScope_, __LINE__)(ctx, label, color)
#define SET_CONTEXT_MARKER_FUNCTION(ctx) SET_CONTEXT_MARKER(ctx, __FUNCTION__)
#define SET_CONTEXT_MARKER_FUNCTION_COLORED(ctx, color) SET_CONTEXT_MARKER_COLORED(ctx, __FUNCTION__, color)
}