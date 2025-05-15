#pragma once

namespace vkr::Render
{
	class Shader
	{
	public:
		Shader();

		//Loads from an already compiled dxil file
		static Shader* LoadFromFile(wstring Path);
		//Compiles at runtime and loads a shader file
		static Shader* CompileFromFile(wstring path, string entrypoint, string shadermodel, bool debug);
	private:
		ComPtr<ID3DBlob> m_ShaderBlob;
	};
}