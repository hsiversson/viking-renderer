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

		bool Init(const RootSignatureDesc& desc);

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;
	};
}