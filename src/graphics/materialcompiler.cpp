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

		std::vector<D3D12_EXPORT_DESC> hitGroupExports;
		hitGroupExports.reserve(2);
		if (addClosestHit)
		{
			D3D12_EXPORT_DESC exportDesc = {};
			exportDesc.Name = uniqueHitGroupClosestHitId.c_str();
			hitGroupExports.push_back(exportDesc);
		}
		if (addAnyHit)
		{
			D3D12_EXPORT_DESC exportDesc = {};
			exportDesc.Name = uniqueHitGroupAnyHitId.c_str();
			hitGroupExports.push_back(exportDesc);
		}

		D3D12_DXIL_LIBRARY_DESC dxilLibDesc = {};
		dxilLibDesc.DXILLibrary.pShaderBytecode = hitGroupShaderBytecode->GetByteCode();
		dxilLibDesc.DXILLibrary.BytecodeLength = hitGroupShaderBytecode->GetByteCodeSize();
		dxilLibDesc.NumExports = hitGroupExports.size();
		dxilLibDesc.pExports = hitGroupExports.data();

		D3D12_STATE_SUBOBJECT dxilLibSubobject = {};
		dxilLibSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		dxilLibSubobject.pDesc = &dxilLibDesc;

		D3D12_HIT_GROUP_DESC hitGroupDesc = {};
		hitGroupDesc.HitGroupExport = uniqueHitGroupId.c_str();
		hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
		if (addClosestHit)
		{
			hitGroupDesc.ClosestHitShaderImport = uniqueHitGroupClosestHitId.c_str();
		}
		if (addAnyHit)
		{
			hitGroupDesc.AnyHitShaderImport = uniqueHitGroupClosestHitId.c_str();
		}

		D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
		hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		hitGroupSubobject.pDesc = &hitGroupDesc;

		return false;
	}

}