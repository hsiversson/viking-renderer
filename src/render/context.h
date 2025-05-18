#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	class Buffer;
	class PipelineState;
	class RootSignature;

	class Context
	{
	public:
		Context();
		~Context();

		void Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator);

		void Dispatch(const Vector3i& Groups);
		void BindPSO(PipelineState* pipelineState, RootSignature* rootSignature);
		void BindRootConstantBuffers(std::vector<Buffer*> buffers);

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
	};
}