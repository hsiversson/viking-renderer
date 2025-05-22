#pragma once
#include "render/deviceobject.h"
#include "render/rendercommon.h"

namespace vkr::Render
{
	class Buffer;
	class PipelineState;
	class RootSignature;

	enum ContextType
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

		ComPtr<ID3D12GraphicsCommandList> m_CommandList;
		ComPtr<ID3D12CommandAllocator> m_CommandAllocator;

		DrawState CurrentState;
		DrawState NewState;
		bool m_StateUpdate = false;

		const ContextType m_Type;
	};
}