#pragma once
#include "pipelinestate.h"

namespace vkr::Render
{
	struct RootSignatureDesc
	{
		PipelineStateType m_PipelineUsage;
		uint32_t m_NumConstantBufferSlots;
	};

	class RootSignature
	{
	public:
		RootSignature();
		~RootSignature();

		bool Init(const RootSignatureDesc& desc, ID3D12Device* Device);
		ID3D12RootSignature* GetD3DRootSignature() { return m_RootSignature.Get(); }

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;
	};
}