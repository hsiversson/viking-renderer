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

		for (const std::filesystem::path& texturePath : desc.m_TexturePaths)
		{
			Ref<Render::Texture> texture = device->LoadTexture(texturePath);
			m_Textures.push_back(device->CreateTextureView(Render::TextureViewDesc{}, texture));
		}

		m_FrontCounterClockwise = desc.m_FrontCounterClockwise;
		m_TwoSided = desc.m_TwoSided;
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

	Render::TextureView* Material::GetTexture(uint32_t index) const
	{
		if (m_Textures.empty())
			return nullptr;

		return m_Textures[index].get();
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
				"cbuffer ConstantBuffer : register(b0)\n"
				"{\n"
				"	float4x4 WorldToClip;\n"
				"	float4x4 ModelToWorld;\n"
				"	float3 BaseColor;\n"
				"	uint TextureDescriptor;\n"
				"};\n"
				"struct VSOutput\n"
				"{\n"
				"	float4 clipPosition : SV_POSITION;\n"
				"	float3 normal : NORMAL;\n"
				"	float2 uv : UV;\n"
				"};\n"
				"VSOutput MainVS(VSInput input)\n"
				"{\n"
				"	VSOutput output;\n"
				"	float4 worldPosition = mul(ModelToWorld, float4(input.position0, 1.0f));\n"
				"	output.clipPosition = mul(WorldToClip, worldPosition);\n"
				"	output.normal = input.normal0;\n"
				"	output.uv = input.uv0;\n"
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
			psoDesc.Default.m_RenderTargetState = { {Render::Format::FORMAT_RGB10A2_UNORM} };

			if (depthOnly)
				psoDesc.Default.m_DepthStencilState = { true, true, Render::COMPARISON_FUNC_GREATER_EQUAL, Render::Format::FORMAT_D32_FLOAT };
			else
				psoDesc.Default.m_DepthStencilState = { true, false, Render::COMPARISON_FUNC_EQUAL, Render::Format::FORMAT_D32_FLOAT };

			psoDesc.Default.m_BlendState.RTBlends.push_back({ true, Render::BLEND_OP_ADD, Render::BLEND_SRC_ALPHA, Render::BLEND_INV_SRC_ALPHA, Render::BLEND_OP_ADD, Render::BLEND_ONE, Render::BLEND_ZERO, Render::COLOR_WRITE_ALL });
			psoMap[vertexLayout] = device->CreatePipelineState(psoDesc);
		}

		return psoMap.at(vertexLayout);
	}

}