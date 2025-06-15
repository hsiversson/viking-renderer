#pragma once
#include "render/rendercommon.h"
#include "render/shader.h"

struct IDxcCompiler3;
struct IDxcUtils;
struct IDxcIncludeHandler;

namespace vkr::Render
{
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		~ShaderCompiler();

		bool CompileFromFile(Shader& outShader, const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_6);
		bool CompileFromMemory(Shader& outShader, const std::string& shaderSource, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel = ShaderModel::SM_6_6);

	private:
		ComPtr<IDxcCompiler3> m_Compiler;
		ComPtr<IDxcUtils> m_Utils;
		ComPtr<IDxcIncludeHandler> m_IncludeHandler;
	};
}