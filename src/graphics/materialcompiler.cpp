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

	bool MaterialCompiler::Compile(Material& outMaterial)
	{
		return false;
	}

	bool MaterialCompiler::CompileRaytracingHitGroup(Material& outMaterial)
	{
		bool addClosestHit = true;
		bool addAnyHit = true;

		const std::wstring uniqueHitGroupId;
		const std::wstring uniqueHitGroupClosestHitId = uniqueHitGroupId + L"_ClosestHit";
		const std::wstring uniqueHitGroupAnyHitId = uniqueHitGroupId + L"_AnyHit";

		std::wstringstream shaderCode;
		if (addClosestHit)
		{
			shaderCode <<
				"[shader(\"closesthit\")]\n"
				"void " << uniqueHitGroupId << "_ClosestHit(inout RaytracingPayload payload, in BuiltInTriangleIntersectionAttributes intersectionAttributes)\n"
				"{\n"
				"	payload.irradiance = float3(1.0f, 0.0f, 1.0f);\n"
				"}\n";
		}
		if (addAnyHit)
		{
			shaderCode <<
				"[shader(\"anyhit\")]\n"
				"void " << uniqueHitGroupId << "_AnyHit(inout RaytracingPayload payload, in BuiltInTriangleIntersectionAttributes intersectionAttributes)\n"
				"{\n"
				"	payload.irradiance = float3(0.0f, 1.0f, 0.0f);\n"
				"}\n";
		}

		Ref<Render::Shader> hitGroupShaderBytecode = Render::GetDevice()->CreateShaderFromString(shaderCode.str(), nullptr, Render::SHADER_STAGE_RAYTRACING);
		if (!hitGroupShaderBytecode)
		{
			return false;
		}

		Render::RaytracingHitGroupDesc hitGroupDesc;
		hitGroupDesc.m_Shader = hitGroupShaderBytecode.get();

		return false;
	}

}