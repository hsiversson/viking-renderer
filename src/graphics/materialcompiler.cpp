#include "materialcompiler.h"
#include "material.h"

#include "render/device.h"

namespace vkr::Graphics
{

	MaterialCompiler::MaterialCompiler()
	{

	}

	MaterialCompiler::~MaterialCompiler()
	{

	}

	bool MaterialCompiler::Compile(const Render::VertexLayout* vertexLayout, Material* outMaterial)
	{
		m_CurrentVertexLayout = vertexLayout;
		m_CurrentMaterial = outMaterial;

		if (!CompileRaytracingHitGroup())
			return false;

		m_CurrentVertexLayout = nullptr;
		m_CurrentMaterial = nullptr;
		return true;
	}

	std::string MaterialCompiler::GenerateInstanceDataStruct()
	{
		std::ostringstream ss;
		ss << "    float4x4 localToWorld;\n";
		ss << "    float4x4 prevLocalToWorld;\n"; // TODO: only if material write velocity?
		ss << "    uint materialId;\n";
		ss << "    uint indexBufferDescriptorIndex;\n";
		ss << "    uint indexStride;\n";
		ss << "    uint vertexBufferDescriptorIndex;\n";
		ss << "    uint vertexStride;\n";

		for (auto& attribute : m_CurrentVertexLayout->GetAttributes())
		{
			if (attribute.m_Index > 0)
			{
				ss << "    uint vertex" << attribute.GetTypeVariableName(attribute.m_Type) << attribute.m_Index << "ByteOffset;\n";
			}
			else
			{
				ss << "    uint vertex" << attribute.GetTypeVariableName(attribute.m_Type) << "ByteOffset;\n";
			}
		}

		return ss.str();
	}

	std::string MaterialCompiler::GenerateMaterialParametersStruct()
	{
		std::ostringstream ss;
		for (const MaterialParameterDesc& param : m_CurrentMaterial->GetParameters())
		{
			ss << "    " << param.GetHLSLType() << " " << param.m_Identifier << ";\n";
		}
		return ss.str();
	}

	std::string MaterialCompiler::GeneratePackedMaterialParametersStruct()
	{
		std::ostringstream ss;
		for (const MaterialParameterDesc& param : m_CurrentMaterial->GetParameters())
		{
			ss << "    " << param.GetPackedHLSLType() << " " << param.m_Identifier << ";\n";
		}
		return ss.str();
	}

	std::string MaterialCompiler::GenerateResolveMaterialParametersCode()
	{
		std::ostringstream ss;
		for (const MaterialParameterDesc& param : m_CurrentMaterial->GetParameters())
		{
			if (param.m_Type == MaterialParameterType::Texture)
			{
				ss << "    resolvedMaterialParams." << param.m_Identifier << " = ResourceDescriptorHeap[NonUniformResourceIndex(packedParams." << param.m_Identifier << ")];\n";
			}
			else if (param.m_Type == MaterialParameterType::Sampler)
			{
				ss << "    resolvedMaterialParams." << param.m_Identifier << " = SamplerDescriptorHeap[NonUniformResourceIndex(packedParams." << param.m_Identifier << ")];\n";
			}
			else
			{
				ss << "    resolvedMaterialParams." << param.m_Identifier << " = packedParams." << param.m_Identifier << ";\n";
			}
		}
		return ss.str();
	}

	std::string MaterialCompiler::GenerateResolvedHitInfoStruct()
	{
		std::ostringstream ss;
		ss << "    float mipLevel;\n";
		for (auto& attribute : m_CurrentVertexLayout->GetAttributes())
		{
			const std::string indexStr = attribute.m_Index > 0 ? std::to_string(attribute.m_Index) : "";
			ss << "    " << GetFormatHLSLName(attribute.m_Format) << " " << attribute.GetTypeVariableName(attribute.m_Type) << indexStr << ";\n";
		}
		return ss.str();
	}

	std::string MaterialCompiler::GenerateResolveHitCode()
	{
		std::ostringstream ss;
		ss << "    ByteAddressBuffer vertexBuffer = ResourceDescriptorHeap[NonUniformResourceIndex(instanceData.vertexBufferDescriptorIndex)];\n";
		ss << "    ResolvedHitInfo v[3];\n";
		ss << "    [unroll(3)]\n";
		ss << "    for (uint i = 0; i < 3; ++i)\n";
		ss << "    {\n";
		ss << "        const uint vertexByteOffset = indices[i] * instanceData.vertexStride;\n";
		for (auto& attribute : m_CurrentVertexLayout->GetAttributes())
		{
			const std::string indexStr = attribute.m_Index > 0 ? std::to_string(attribute.m_Index) : "";
			const std::string attributeName = std::format("{}{}", attribute.GetTypeVariableName(attribute.m_Type), indexStr);
			ss << "        v[i]." << attributeName << " = vertexBuffer.Load<" << GetFormatHLSLName(attribute.m_Format) << ">(vertexByteOffset + instanceData.vertex" << attributeName << "ByteOffset);\n";
		}
		ss << "    }\n";
		for (auto& attribute : m_CurrentVertexLayout->GetAttributes())
		{
			const std::string indexStr = attribute.m_Index > 0 ? std::to_string(attribute.m_Index) : "";
			const std::string attributeName = std::format("{}{}", attribute.GetTypeVariableName(attribute.m_Type), indexStr);
			ss << "    resolvedHitInfo." << attributeName << " = BarycentricLerp(v[0]." << attributeName << ", v[1]." << attributeName << ", v[2]." << attributeName << ", barycentrics);\n";
		}

		bool computeMipLevel = true; // TODO: only if we sample textures?
		if (computeMipLevel)
		{
			ss << "    float3 dPos1 = v[1].Position - v[0].Position;\n";
			ss << "    float3 dPos2 = v[2].Position - v[0].Position;\n";
			ss << "    float2 dUV1 = v[1].UV - v[0].UV;\n";
			ss << "    float2 dUV2 = v[2].UV - v[0].UV;\n";
			ss << "    float3 dpdu = normalize(dUV2.y * dPos1 - dUV1.y * dPos2);\n";
			ss << "    float3 dpdv = normalize(-dUV2.x * dPos1 + dUV1.x * dPos2);\n";
			ss << "    float footprint = max(length(cross(dpdu, dpdv)), FLT_EPSILON_VALUE);\n";
			ss << "    float mipLevel = 0.5 * log2(footprint);\n";
			ss << "    resolvedHitInfo.mipLevel = max(mipLevel, 0.0f);\n";
		}

		return ss.str();
	}

	std::string MaterialCompiler::GenerateResolveMaterialCode()
	{
		// TODO: base below on material setup

		std::ostringstream ss;
		ss << "    resolvedMaterial.WorldPosition = mul(instanceData.localToWorld, float4(hitInfo.Position, 1.0f)).xyz;\n";
		ss << "    resolvedMaterial.Albedo = materialParameters.albedoTexture.SampleLevel(g_SamplerBilinearClamp, hitInfo.UV, hitInfo.mipLevel).rgb;\n";
		ss << "    resolvedMaterial.Emission = materialParameters.emissiveTexture.SampleLevel(g_SamplerBilinearClamp, hitInfo.UV, hitInfo.mipLevel).rgb;\n";
		ss << "    float2 encodedNormal = materialParameters.normalTexture.SampleLevel(g_SamplerBilinearClamp, hitInfo.UV, hitInfo.mipLevel).rg * 2.0f - 1.0f;\n";
		ss << "    float nx = encodedNormal.x;\n";
		ss << "    float ny = encodedNormal.y;\n";
		ss << "    float nz = sqrt(saturate(1.0f - nx * nx - ny * ny));\n";
		ss << "    float3 detailNormal = float3(nx, -ny, nz);\n";
		ss << "    float3 normal = normalize(hitInfo.Normal);\n";
		ss << "    float3 tangent = normalize(hitInfo.Tangent.xyz);\n";
		ss << "    float3 binormal = cross(normal, tangent) * hitInfo.Tangent.w;\n";
		ss << "    float3x3 tangentToLocal = float3x3(tangent.x, binormal.x, normal.x, tangent.y, binormal.y, normal.y, tangent.z, binormal.z, normal.z);\n";
		ss << "    float3 localNormal = mul(tangentToLocal, detailNormal);\n";
		ss << "    resolvedMaterial.WorldNormal = normalize(mul(instanceData.localToWorld, float4(localNormal, 0)).xyz);\n";
		ss << "    float4 material = materialParameters.materialTexture.SampleLevel(g_SamplerBilinearClamp, hitInfo.UV, hitInfo.mipLevel);\n";
		ss << "    resolvedMaterial.Roughness = material.g;\n";
		ss << "    resolvedMaterial.Metallic = material.b;\n";
		ss << "    resolvedMaterial.AO = 1.0f;\n";

		return ss.str();
	}

	bool MaterialCompiler::CompileRaytracingHitGroup()
	{
		bool addClosestHit = true; // TODO: for opaque materials?
		bool addAnyHit = false; // TODO: for transparent materials?

		m_CurrentMaterial->m_HitGroupIdentifier = std::format("MaterialHitGroup_{}", reinterpret_cast<uintptr_t>(m_CurrentMaterial));

		std::string hitGroupCode = ReadFileToString(SystemPaths::GetInContentDirectory(CONTENT_DIRECTORY_ENGINE, "shaders/materialhitgrouptemplate.hlsl"));

		if (addClosestHit)
		{
			m_CurrentMaterial->m_ClosestHitIdentifier = m_CurrentMaterial->m_HitGroupIdentifier + "_ClosestHit";
			hitGroupCode.insert(0, "#define HAS_CLOSEST_HIT 1\n");
			ReplaceString("$CLOSESTHIT_IDENTIFIER$", m_CurrentMaterial->m_ClosestHitIdentifier, hitGroupCode);
		}

		if (addAnyHit)
		{
			m_CurrentMaterial->m_AnyHitIdentifier = m_CurrentMaterial->m_HitGroupIdentifier + "_AnyHit";
			hitGroupCode.insert(0, "#define HAS_ANY_HIT 1\n");
			ReplaceString("$ANYHIT_IDENTIFIER$", m_CurrentMaterial->m_AnyHitIdentifier, hitGroupCode);
		}

		ReplaceString("$INSTANCE_DATA$", GenerateInstanceDataStruct(), hitGroupCode);
		ReplaceString("$MATERIAL_PARAMETERS$", GenerateMaterialParametersStruct(), hitGroupCode);
		ReplaceString("$PACKED_MATERIAL_PARAMETERS$", GeneratePackedMaterialParametersStruct(), hitGroupCode);
		ReplaceString("$RESOLVE_MATERIAL_PARAMETERS$", GenerateResolveMaterialParametersCode(), hitGroupCode);
		ReplaceString("$RESOLVED_HIT_INFO$", GenerateResolvedHitInfoStruct(), hitGroupCode);
		ReplaceString("$RESOLVE_HIT$", GenerateResolveHitCode(), hitGroupCode);
		ReplaceString("$RESOLVE_MATERIAL$", GenerateResolveMaterialCode(), hitGroupCode);

		m_CurrentMaterial->m_HitGroupShader = Render::GetDevice()->CreateShaderFromString(hitGroupCode, nullptr, Render::SHADER_STAGE_RAYTRACING);
		if (!m_CurrentMaterial->m_HitGroupShader)
		{
			return false;
		}

		return true;
	}

	bool MaterialCompiler::CompilePixelShader()
	{
		return false;
	}

	bool MaterialCompiler::CompileVertexShader()
	{
		return false;
	}

}