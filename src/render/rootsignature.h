#pragma once
#include "render/pipelinestate.h"

namespace vkr::Render
{
	struct ConstantBufferDescription
	{
		uint32_t m_Slot = 0;
		uint32_t m_Space = 0;
	};

	struct RootSignatureDesc
	{
		PipelineStateType m_PipelineUsage;
		std::vector<ConstantBufferDescription> m_ConstantBuffers;
	};

	class RootSignature
	{
	public:
		RootSignature();
		~RootSignature();

		bool Init(const RootSignatureDesc& desc);
		ID3D12RootSignature* GetD3DRootSignature() { return m_RootSignature.Get(); }

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;
	};
}