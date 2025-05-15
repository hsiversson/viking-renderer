#include "shader.h"
#include <fstream>

bool vkr::Render::Shader::LoadFromFile(wstring Path)
{
	std::ifstream file(Path, std::ios::binary | std::ios::ate)
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	ComPtr<ID3DBlob> blob;
	if (!D3DCreateBlob(size, blob))
	{
		file.close();
		return nullptr;
	}

	if (!file.read(reinterpret_cast<char*>(blob->GetBufferPointer()), size))
	{
		blob->Release();
		blob = nullptr;
		return nullptr;
	}

	auto result = new Shader();
	result->m_ShaderBlob = blob;
	return result;
}

vkr::Render::Shader* vkr::Render::Shader::CompileFromFile(wstring path, string entrypoint, string shadermodel, bool debug)
{
	ComPtr<ID3DBlob> blob;

	if (debug)
	{
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
	}
	else
	{
		UINT compileFlags = 0;
	}


	if (!D3DCompileFromFile(path.c_str(), nullptr, nullptr, entrypoint, shadermodel, compileFlags, 0, &blob, nullptr))
	{
		return nullptr;
	}

	auto result = new Shader();
	result->m_ShaderBlob = blob;
	return result;
}

