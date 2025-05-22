#include "rootsignature.h"

#include "rendercommon.h"
#include "device.h"

namespace vkr::Render
{
	RootSignature::RootSignature(Device& device)
		: DeviceObject(device)
	{

	}

	RootSignature::~RootSignature()
	{

	}

	bool RootSignature::Init(const RootSignatureDesc& desc)
	{
		D3D12_ROOT_PARAMETER* RootParams = new D3D12_ROOT_PARAMETER[desc.m_NumConstantBufferSlots];
		for (int i = 0; i < desc.m_NumConstantBufferSlots; i++)
		{
			RootParams[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			RootParams[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			RootParams[i].Descriptor.ShaderRegister = i;
			RootParams[i].Descriptor.RegisterSpace = 0;
		}
		D3D12_ROOT_SIGNATURE_DESC Desc;
		Desc.NumParameters = desc.m_NumConstantBufferSlots;
		Desc.pParameters = RootParams;
		Desc.NumStaticSamplers = 0;
		Desc.pStaticSamplers = nullptr;
		Desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		switch (desc.m_PipelineUsage)
		{
		case PIPELINE_STATE_TYPE_DEFAULT:
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			break;
		case PIPELINE_STATE_TYPE_COMPUTE:
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			Desc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			break;
		}

		ComPtr<ID3DBlob> SerializedRootSignature;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &SerializedRootSignature, &ErrorBlob);
		delete[]RootParams;
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			return false;
		}

		hr = m_Device.GetD3DDevice()->CreateRootSignature(0, SerializedRootSignature->GetBufferPointer(), SerializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}
}