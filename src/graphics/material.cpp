#include "material.h"
#include "render/texture.h"
#include "render/pipelinestate.h"
#include "render/device.h"
#include "materialdatabuffer.h"

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
		for (const auto& param : desc.m_Parameters)
		{
			AddParameter(param.first, param.second);
		}

		Render::Device* device = Render::GetDevice();
		std::string shaderCode = GenerateGetMaterialParametersFunction();
		shaderCode += ReadFileToString("../../../content/shaders/simpleforwardtestPS.hlsl");
		m_PixelShader = device->CreateShaderFromString(shaderCode, L"MainPS", vkr::Render::SHADER_STAGE_PIXEL);

		m_BlendMode = desc.m_BlendMode;
		m_Type = desc.m_Type;
		m_WriteVelocity = desc.m_WriteVelocity;
		m_TwoSided = desc.m_TwoSided;
		m_FrontCounterClockwise = desc.m_FrontCounterClockwise;
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

	std::string Material::GenerateGetMaterialParametersFunction()
	{
		std::ostringstream ss;

		ss << "#include \"sceneconstants.hlsl\"\n";

		ss << "struct MaterialParameters\n";
		ss << "{\n";
		for (const MaterialParameterDesc& param : m_Parameters)
		{
			ss << "    " << param.GetHLSLType() << " " << param.m_Identifier << ";\n";
		}
		ss << "};\n\n";
		ss << "struct PackedMaterialParameters\n";
		ss << "{\n";
		for (const MaterialParameterDesc& param : m_Parameters)
		{
			ss << "    " << param.GetPackedHLSLType() << " " << param.m_Identifier << ";\n";
		}
		ss << "};\n\n";

		ss << "MaterialParameters LoadMaterialParameters(uint offset)\n";
		ss << "{\n";
		ss << "    ByteAddressBuffer materialDataBuffer = ResourceDescriptorHeap[SceneConstants.MaterialDataBufferDescriptorIndex];\n";
		ss << "    PackedMaterialParameters packedParams = materialDataBuffer.Load<PackedMaterialParameters>(offset);\n";
		ss << "    MaterialParameters result;\n";
		for (const MaterialParameterDesc& param : m_Parameters)
		{
			if (param.m_Type == MaterialParameterType::Texture)
			{
				ss << "    result." << param.m_Identifier << " = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams." << param.m_Identifier << ")];\n";
			}
			else if (param.m_Type == MaterialParameterType::Sampler)
			{
				ss << "    result." << param.m_Identifier << " = SamplerDescriptorHeap[NonUniformResourceIndex(packedParams." << param.m_Identifier << ")];\n";
			}
			else
			{
				ss << "    result." << param.m_Identifier << " = packedParams." << param.m_Identifier << ";\n";
			}
		}
		ss << "    return result;\n";
		ss << "}\n\n";

		return ss.str();
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
				"	output.clipPosition = mul(SceneConstants.WorldToClip, float4(output.worldPosition, 1.0f));\n"
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
			psoDesc.Default.m_RasterizerState.m_CullMode = m_TwoSided ? Render::FACE_CULL_MODE_NONE : Render::FACE_CULL_MODE_BACK;
			psoDesc.Default.m_RasterizerState.m_FrontIsCounterClockwise = m_FrontCounterClockwise;
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

			if (m_BlendMode == MaterialBlendMode::Translucent)
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
		assert(m_Material && "Material template is required.");
		assert(m_Material->FindParameter(identifier) != nullptr);
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

	uint32_t MaterialInstance::GatherMaterialData(MaterialDataBuffer& materialDataBuffer)
	{
		assert(m_Material && "Material template is required.");
		const std::vector<MaterialParameterDesc>& parameters = m_Material->GetParameters();

		std::vector<uint8_t> materialData;
		for (uint32_t i = 0; i < parameters.size(); ++i)
		{
			const MaterialParameterDesc& paramDesc = parameters[i];
			const MaterialParameterValue* paramValue = GetParameterValue(paramDesc.m_Identifier);

			switch (paramValue->GetType())
			{
			case MaterialParameterType::StaticBool:
			{
				const uint32_t value = static_cast<uint32_t>(paramValue->Get<bool>());
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(uint32_t));
				break;
			}
			case MaterialParameterType::Float:
			{
				const float value = paramValue->Get<float>();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(float));
				break;
			}
			case MaterialParameterType::Float2:
			{
				const Vector2f value = paramValue->Get<Vector2f>();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(Vector2f));
				break;
			}
			case MaterialParameterType::Float3:
			{
				const Vector3f value = paramValue->Get<Vector3f>();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(Vector3f));
				break;
			}
			case MaterialParameterType::Float4:
			{
				const Vector4f value = paramValue->Get<Vector4f>();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(Vector4f));
				break;
			}
			case MaterialParameterType::Texture:
			{
				const Ref<Render::TextureView>& tex = paramValue->Get<Ref<Render::TextureView>>();
				const uint32_t value = tex->GetIndex();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(uint32_t));
				break;
			}
			case MaterialParameterType::Sampler:
			{
				const Ref<Render::Sampler>& sampler = paramValue->Get<Ref<Render::Sampler>>();
				const uint32_t value = sampler->GetIndex();
				const uint8_t* valuePtr = reinterpret_cast<const uint8_t*>(&value);
				materialData.insert(materialData.end(), valuePtr, valuePtr + sizeof(uint32_t));
				break;
			}
			default:
				assert(false);
				return 0;
			}
		}
		
		return materialDataBuffer.AddData(materialData.size(), materialData.data());
	}

	Material* MaterialInstance::GetMaterial() const
	{
		return m_Material.get();
	}

}