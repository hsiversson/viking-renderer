#pragma once
#include "render/rendercommon.h"
#include "render/event.h"

namespace vkr::Render
{
	class Texture;
	class Buffer;
	class PipelineState;
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

		void Dispatch(const Vector3u& Groups);
		void DispatchThreads(const Vector3u& threads);
		void DispatchThreads(Ref<PipelineState> pipelineState, const Vector3u& threads);
		void BindPSO(Ref<PipelineState> pipelineState);
		void BindRootConstantBuffers(std::vector<Buffer*> buffers);
		void BindVertexBuffers(std::vector<Ref<Buffer>> vertexbuffers);
		void BindIndexBuffer(Ref<Buffer> indexbuffer);
		void BindRenderTargets(std::vector<Ref<ResourceDescriptor>> rtdescriptors);
		void SetDepthStencil(Ref<ResourceDescriptor> dsdescriptor);

		void TextureBarrier(uint32_t numBarriers, const TextureBarrierDesc* barrierDescs);
		void TextureBarrier(const TextureBarrierDesc& barrierDesc);
		void BufferBarrier(uint32_t numBarriers, const BufferBarrierDesc* barrierDescs);
		void BufferBarrier(const BufferBarrierDesc& barrierDesc);
		void GlobalBarrier(uint32_t numBarriers, const GlobalBarrierDesc* barrierDescs);
		void GlobalBarrier(const GlobalBarrierDesc& barrierDesc);

		ContextType GetType() const;

	private:
		struct DrawState
		{
			std::vector<Ref<Buffer>> m_VertexBuffers;
			Ref<Buffer> m_IndexBuffer;
			RootSignature* m_RootSignature = nullptr;
			Ref<PipelineState> m_PipelineState = nullptr;
			std::vector<Buffer*> m_RootCB;
			std::vector<Ref<ResourceDescriptor>> m_RenderTargets;
			Ref<ResourceDescriptor> m_DepthStencil;
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