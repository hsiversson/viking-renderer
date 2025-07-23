#pragma once
#include "render/pipelinestate.h"

namespace vkr::Render
{
	struct RootSignatureDesc
	{
		PipelineStateType m_PipelineUsage = PIPELINE_STATE_TYPE_UNKNOWN;
		uint32_t m_NumLocalConstantBuffers = 0;
	};

	class RootSignature
	{
	public:
		RootSignature();
		~RootSignature();

		bool Init(const RootSignatureDesc& desc);
		ID3D12RootSignature* GetD3DRootSignature() { return m_RootSignature.Get(); }

		uint32_t GetLocalConstantBufferParameterStart() const;
		uint32_t GetNumLocalConstantBuffers() const;

		uint32_t GetGlobalConstantBufferParameterStart() const;

		PipelineStateType GetType() const { return m_Desc.m_PipelineUsage; }
	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;
		RootSignatureDesc m_Desc;
	};
}