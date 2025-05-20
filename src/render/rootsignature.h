#pragma once
#include "render/deviceobject.h"
#include "render/pipelinestate.h"

namespace vkr::Render
{
	struct RootSignatureDesc
	{
		PipelineStateType m_PipelineUsage;
		uint32_t m_NumConstantBufferSlots;
	};

	class RootSignature : public DeviceObject
	{
	public:
		RootSignature(Device& device);
		~RootSignature();

		bool Init(const RootSignatureDesc& desc);
		ID3D12RootSignature* GetD3DRootSignature() { return m_RootSignature.Get(); }

	private:
		ComPtr<ID3D12RootSignature> m_RootSignature;
	};
}