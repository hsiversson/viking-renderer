#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"
#include "render/event.h"

namespace vkr::Render
{
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

	class Context : public DeviceObject
	{
	public:
		Context(Device& device, ContextType type);
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

		std::vector<Ref<CommandList>> m_CommandListsToSubmit;
		Event m_LastFlushEvent;

		DrawState CurrentState;
		DrawState NewState;
		bool m_StateUpdate = false;
		bool m_RenderTargetUpdate = false;

		const ContextType m_Type;
	};
}