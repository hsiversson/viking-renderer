#pragma once

namespace vkr::Render 
{
	struct VertexLayout;
}

namespace vkr::Graphics
{
	class Material;
	class MaterialCompiler
	{
	public:
		MaterialCompiler();
		~MaterialCompiler();

		bool Compile(const Render::VertexLayout* vertexLayout, Material* outMaterial);

	private:
		std::string GenerateInstanceDataStruct();

		std::string GenerateMaterialParametersStruct();
		std::string GeneratePackedMaterialParametersStruct();
		std::string GenerateResolveMaterialParametersCode();

		std::string GenerateResolvedHitInfoStruct();
		std::string GenerateResolveHitCode();

		std::string GenerateResolveMaterialCode();

		bool CompileRaytracingHitGroup();

		bool CompilePixelShader();
		bool CompileVertexShader();

		Material* m_CurrentMaterial;
		const Render::VertexLayout* m_CurrentVertexLayout;
	};
}