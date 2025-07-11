#include "material.h"
#include "render/texture.h"
#include "render/pipelinestate.h"
#include "render/device.h"

namespace vkr::Graphics
{

	Material::Material()
	{

	}

	Material::~Material()
	{

	}

	bool Material::Init(const MaterialDesc& desc)
	{
		Render::Device* device = Render::GetDevice();
		m_PixelShader = device->CreateShader("../../../content/shaders/simpleforwardtestPS.hlsl", L"MainPS", vkr::Render::SHADER_STAGE_PIXEL);

		for (const auto& param : desc.m_Parameters)
		{
			AddParameter(param.first, param.second);
		}

		m_Desc = desc;
		return true;
	}

	Ref<Render::PipelineState> Material::GetDepthPipelineState(const Render::VertexLayout& vertexLayout)
	{
		return GetOrCreatePSO(vertexLayout, true);
	}

	Ref<Render::PipelineState> Material::GetDefaultPipelineState(const Render::VertexLayout& vertexLayout)
	{
		return GetOrCreatePSO(vertexLayout, false);
	}

	void Material::AddParameter(const MaterialParameterDesc& desc, const MaterialParameterValue& defaultValue)
	{
		m_Parameters.push_back(desc);
		m_ParameterValues[desc.m_Identifier] = defaultValue;
	}

	const MaterialParameterDesc* Material::FindParameter(const std::string& identifier) const
	{
		for (const auto& param : m_Parameters) 
		{
			if (param.m_Identifier == identifier)
			{
				return &param;
			}
		}
		return nullptr;
	}

	const MaterialParameterValue* Material::GetParameterValue(const std::string& identifier) const
	{
		auto it = m_ParameterValues.find(identifier);
		return it != m_ParameterValues.end() ? &it->second : nullptr;
	}

	const std::vector<MaterialParameterDesc>& Material::GetParameters() const
	{
		return m_Parameters;
	}

	Ref<Render::PipelineState> Material::GetOrCreatePSO(const Render::VertexLayout& vertexLayout, bool depthOnly)
	{
		CachedPSOs& psoMap = depthOnly ? m_DepthOnlyPSOs : m_DefaultPSOs;
		if (!psoMap.contains(vertexLayout))
		{
			Render::Device* device = Render::GetDevice();

			// Create vertex shader
			std::stringstream vertexShaderCode;
			vertexShaderCode <<
				"#include \"../../../content/shaders/sceneconstants.hlsl\"\n"
				"#include \"../../../content/shaders/instancing.hlsl\"\n"
				"struct VSInput\n"
				"{\n";
			for (const Render::VertexAttribute& attr : vertexLayout.m_Attributes)
			{
				switch (attr.m_Format)
				{
				case Render::FORMAT_R32_FLOAT:
					vertexShaderCode << "float ";
					break;
				case Render::FORMAT_RG32_FLOAT:
					vertexShaderCode << "float2 ";
					break;
				case Render::FORMAT_RGB32_FLOAT:
					vertexShaderCode << "float3 ";
					break;
				case Render::FORMAT_RGBA32_FLOAT:
					vertexShaderCode << "float4 ";
					break;
				case Render::FORMAT_R32_UINT:
					vertexShaderCode << "uint ";
					break;
				case Render::FORMAT_RG32_UINT:
					vertexShaderCode << "uint2 ";
					break;
				case Render::FORMAT_RGB32_UINT:
					vertexShaderCode << "uint3 ";
					break;
				case Render::FORMAT_RGBA32_UINT:
					vertexShaderCode << "uint4 ";
					break;
				default:
					assert(false);
					return nullptr;
				}

				switch (attr.m_Type)
				{
				case Render::VertexAttribute::TYPE_POSITION:
					vertexShaderCode << "position" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_NORMAL:
					vertexShaderCode << "normal" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_TANGENT:
					vertexShaderCode << "tangent" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_UV:
					vertexShaderCode << "uv" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_COLOR:
					vertexShaderCode << "color" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_BONE_INDEX:
					vertexShaderCode << "boneIndex" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				case Render::VertexAttribute::TYPE_BONE_WEIGHT:
					vertexShaderCode << "boneWeight" << attr.m_Index << " : " << Render::VertexAttribute::GetTypeSemantic(attr.m_Type) << attr.m_Index << ";\n";
					break;
				}
			}
			vertexShaderCode << "};\n";

			vertexShaderCode <<
				"cbuffer PerBatchConstantBuffer : register(b0)\n"
				"{\n"
				"	uint BatchInstanceDataOffsetStart;\n"
				"	uint AlbedoTextureDescriptor;\n"
				"	uint NormalTextureDescriptor;\n"
				"	uint MetallicRoughnessTextureDescriptor;\n"
				"	uint RaytracingSceneDescriptor;\n"
				"};\n"
				"struct InstanceData\n"
				"{\n"
				"	float4x4 ModelToWorld;\n"
				"	uint MaterialID;\n"
				"};\n"
				"struct VSOutput\n"
				"{\n"
				"	float4 clipPosition : SV_POSITION;\n"
				"	float3 worldPosition : WORLD_POSITION;\n"
				"	float3 normal : NORMAL;\n"
				"	float4 tangent : TANGENT;"
				"	float2 uv : UV;\n"
				"	nointerpolation uint instanceID : INSTANCE_ID;\n"
				"};\n"
				"VSOutput MainVS(VSInput input, uint instanceID : SV_InstanceID)\n"
				"{\n"
				"	VSOutput output;\n"
				"	InstanceData data = GetInstanceData<InstanceData>(BatchInstanceDataOffsetStart, instanceID);"
				"	output.worldPosition = mul(data.ModelToWorld, float4(input.position0, 1.0f)).xyz;\n"
				"	output.clipPosition = mul(WorldToClip, float4(output.worldPosition, 1.0f));\n"
				"	output.normal = input.normal0;\n"
				"	output.tangent = input.tangent0;\n"
				"	output.uv = input.uv0;\n"
				"	output.instanceID = instanceID;\n"
				"	return output;\n"
				"};";

			Ref<Render::Shader> vertexShader = device->CreateShaderFromString(vertexShaderCode.str(), L"MainVS", Render::SHADER_STAGE_VERTEX);

			Render::PipelineStateDesc psoDesc = {};
			psoDesc.m_Type = Render::PIPELINE_STATE_TYPE_DEFAULT;
			psoDesc.Default.m_PrimitiveType = Render::PRIMITIVE_TYPE_TRIANGLE;
			psoDesc.Default.m_VertexShader = vertexShader.get();
			psoDesc.Default.m_PixelShader = depthOnly ? nullptr : m_PixelShader.get();
			psoDesc.Default.m_VertexLayout = vertexLayout;
			psoDesc.Default.m_RasterizerState.m_CullMode = m_Desc.m_TwoSided ? Render::FACE_CULL_MODE_NONE : Render::FACE_CULL_MODE_BACK;
			psoDesc.Default.m_RasterizerState.m_FrontIsCounterClockwise = m_Desc.m_FrontCounterClockwise;
			psoDesc.Default.m_RenderTargetState.m_Formats[0] = Render::Format::FORMAT_RGB10A2_UNORM;

			if (depthOnly)
			{
				psoDesc.Default.m_DepthStencilState.m_Enabled = true;
				psoDesc.Default.m_DepthStencilState.m_WriteDepth = true;
				psoDesc.Default.m_DepthStencilState.m_ComparisonFunc = Render::COMPARISON_FUNC_GREATER_EQUAL;
				psoDesc.Default.m_DepthStencilState.m_DSFormat = Render::Format::FORMAT_D32_FLOAT;
			}
			else
			{
				psoDesc.Default.m_DepthStencilState.m_Enabled = true;
				psoDesc.Default.m_DepthStencilState.m_WriteDepth = false;
				psoDesc.Default.m_DepthStencilState.m_ComparisonFunc = Render::COMPARISON_FUNC_EQUAL;
				psoDesc.Default.m_DepthStencilState.m_DSFormat = Render::Format::FORMAT_D32_FLOAT;
			}

			if (m_Desc.m_BlendMode == MaterialBlendMode::Translucent)
			{
				psoDesc.Default.m_BlendState.RTBlends[0].m_Enabled = true;
				psoDesc.Default.m_BlendState.RTBlends[0].m_Operation = Render::BLEND_OP_ADD;
				psoDesc.Default.m_BlendState.RTBlends[0].m_SrcBlend = Render::BLEND_SRC_ALPHA;
				psoDesc.Default.m_BlendState.RTBlends[0].m_DstBlend = Render::BLEND_INV_SRC_ALPHA;
				psoDesc.Default.m_BlendState.RTBlends[0].m_AlphaOperation = Render::BLEND_OP_ADD;
				psoDesc.Default.m_BlendState.RTBlends[0].m_SrcBlendAlpha = Render::BLEND_ONE;
				psoDesc.Default.m_BlendState.RTBlends[0].m_DstBlendAlpha = Render::BLEND_ZERO;
				psoDesc.Default.m_BlendState.RTBlends[0].m_WriteMask = Render::COLOR_WRITE_ALL;
			}
			else
			{
				psoDesc.Default.m_BlendState.RTBlends[0].m_Enabled = false;
			}

			psoMap[vertexLayout] = device->CreatePipelineState(psoDesc);
		}

		return psoMap.at(vertexLayout);
	}

	MaterialInstance::MaterialInstance(const Ref<Material>& material)
		: m_Material(material)
	{
	}

	void MaterialInstance::SetParameterValue(const std::string& identifier, const MaterialParameterValue& value)
	{
		m_ParameterOverrides[identifier] = value;
	}

	const MaterialParameterValue* MaterialInstance::GetParameterValue(const std::string& identifier) const
	{
		auto it = m_ParameterOverrides.find(identifier);
		if (it != m_ParameterOverrides.end())
		{
			return &it->second;
		}

		if (m_Material)
		{
			return m_Material->GetParameterValue(identifier);
		}

		return nullptr;
	}

	const MaterialParameterDesc* MaterialInstance::GetParameter(const std::string& identifier) const
	{
		return m_Material ? m_Material->FindParameter(identifier) : nullptr;
	}

	const std::vector<MaterialParameterDesc>& MaterialInstance::GetParameters() const
	{
		assert(m_Material && "Material template is required.");
		return m_Material->GetParameters();
	}

}