#include "rootsignature.h"

#include "rendercommon.h"

namespace vkr::Render
{
	RootSignature::RootSignature()
	{

	}

	RootSignature::~RootSignature()
	{

	}

	bool RootSignature::Init(const RootSignatureDesc& desc, ID3D12Device* Device)
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
		Desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; //We need this? Or maybe this D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE if its for raytracing?

		ComPtr<ID3DBlob> SerializedRootSignature;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &SerializedRootSignature, &ErrorBlob);
		delete[]RootParams;
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			return false;
		}

		hr = Device->CreateRootSignature(0, SerializedRootSignature->GetBufferPointer(), SerializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}
}