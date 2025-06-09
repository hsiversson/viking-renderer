#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"
#include "render/event.h"

namespace vkr::Render
{
	class Texture;
	class Buffer;
	class PipelineState;
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

	class Context : public DeviceObject
	{
	public:
		Context(Device& device, ContextType type);
		~Context();

		void Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator);

		void Begin();
		void End();
		Event Flush();

		void Dispatch(const Vector3u& Groups);
		void DispatchThreads(const Vector3u& threads);
		void DispatchThreads(PipelineState* pipelineState, const Vector3u& threads);
		void BindPSO(PipelineState* pipelineState);
		void BindRootConstantBuffers(std::vector<Buffer*> buffers);

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
			RootSignature* m_RootSignature = nullptr;
			PipelineState* m_PipelineState = nullptr;
			std::vector<Buffer*> m_RootCB;
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

		const ContextType m_Type;
	};
}