#pragma once
#include "render/rendercommon.h"
#include "render/event.h"

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

	class Context
	{
	public:
		Context(ContextType type);
		~Context();

		void Begin();
		void End();
		Event Flush();

		// Compute
		void Dispatch(const Vector3u& Groups);
		void DispatchThreads(const Vector3u& threads);
		void DispatchThreads(Ref<PipelineState> pipelineState, const Vector3u& threads);

		//Draw
		void DrawIndexed(uint32_t StartIndex, uint32_t StartVertex);

		//Render state
		void BindPSO(Ref<PipelineState> pipelineState);
		void BindRootConstantBuffers(Buffer** buffers, size_t bufferCount, uint64_t* offsets = nullptr);
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

		ContextType GetType() const;

	private:
		struct DrawState
		{
			PrimitiveTopology m_Topology;
			std::vector<Ref<Buffer>> m_VertexBuffers;
			Ref<Buffer> m_IndexBuffer;
			RootSignature* m_RootSignature = nullptr;
			Ref<PipelineState> m_PipelineState = nullptr;
			std::vector<Buffer*> m_RootCB;
			std::vector<uint64_t> m_RootCBOffsets;
			std::vector<Ref<RenderTargetView>> m_RenderTargets;
			Ref<DepthStencilView> m_DepthStencil;
		};

		void UpdateState();

		Ref<CommandList> m_CommandList;
		ID3D12GraphicsCommandList* m_CurrentD3DCommandList;
		ID3D12GraphicsCommandList7* m_CurrentD3DCommandList7;

		std::vector<Ref<CommandList>> m_CommandListsToSubmit;
		Event m_LastFlushEvent;

		DrawState CurrentState;
		DrawState NewState;
		bool m_StateUpdate = false;
		bool m_RenderTargetUpdate = false;

		const ContextType m_Type;
	};
}