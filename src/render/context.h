#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"
#include "render/event.h"

namespace vkr::Render
{
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

		std::vector<Ref<CommandList>> m_CommandListsToSubmit;
		Event m_LastFlushEvent;

		DrawState CurrentState;
		DrawState NewState;
		bool m_StateUpdate = false;

		const ContextType m_Type;
	};
}