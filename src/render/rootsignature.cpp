#include "rootsignature.h"

#include "rendercommon.h"
#include "device.h"

namespace vkr::Render
{
	RootSignature::RootSignature()
		: m_Desc{}
	{

	}

	RootSignature::~RootSignature()
	{

	}

	bool RootSignature::Init(const RootSignatureDesc& desc)
	{
		assert(desc.m_NumLocalConstantBuffers <= MAX_NUM_LOCAL_CONSTANT_BUFFERS && "Cannot have more than 4 local constant buffers");

		std::vector<D3D12_ROOT_PARAMETER> rootParams;
		rootParams.reserve(desc.m_NumLocalConstantBuffers + GLOBAL_CONSTANT_BUFFER_COUNT);

		for (uint32_t i = 0; i < desc.m_NumLocalConstantBuffers; i++)
		{
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			param.Descriptor.ShaderRegister = i;
			param.Descriptor.RegisterSpace = 0;
			rootParams.push_back(param);
		}

		for (uint32_t i = 0; i < GLOBAL_CONSTANT_BUFFER_COUNT; ++i)
		{
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			param.Descriptor.ShaderRegister = i;
			param.Descriptor.RegisterSpace = 1;
			rootParams.push_back(param);
		}

		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.NumParameters = rootParams.size();
		rootSignatureDesc.pParameters = rootParams.data();

		D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[2];
		staticSamplerDescs[0] = {};
		staticSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		staticSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplerDescs[0].ShaderRegister = 0;
		staticSamplerDescs[0].RegisterSpace = 0;
		staticSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		staticSamplerDescs[1] = {};
		staticSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		staticSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		staticSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
		staticSamplerDescs[1].ShaderRegister = 1;
		staticSamplerDescs[1].RegisterSpace = 0;
		staticSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

		rootSignatureDesc.NumStaticSamplers = 2;
		rootSignatureDesc.pStaticSamplers = staticSamplerDescs;

		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
		rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
		rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

		switch (desc.m_PipelineUsage)
		{
		case PIPELINE_STATE_TYPE_DEFAULT:
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			break;
		case PIPELINE_STATE_TYPE_COMPUTE:
		case PIPELINE_STATE_TYPE_RAYTRACING:
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
			rootSignatureDesc.Flags |= D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
			break;
		}

		ComPtr<ID3DBlob> SerializedRootSignature;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SerializedRootSignature, &ErrorBlob);
		if (FAILED(hr))
		{
			OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
			return false;
		}

		hr = GetDevice()->GetD3DDevice()->CreateRootSignature(0, SerializedRootSignature->GetBufferPointer(), SerializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
		if (FAILED(hr))
		{
			return false;
		}

		m_Desc = desc;
		return true;
	}

	uint32_t RootSignature::GetLocalConstantBufferParameterStart() const
	{
		return 0;
	}

	uint32_t RootSignature::GetNumLocalConstantBuffers() const
	{
		return m_Desc.m_NumLocalConstantBuffers;
	}

	uint32_t RootSignature::GetGlobalConstantBufferParameterStart() const
	{
		return m_Desc.m_NumLocalConstantBuffers;
	}
}