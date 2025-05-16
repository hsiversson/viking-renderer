#include "shader.h"

namespace vkr::Render
{
	Shader::Shader()
	{
	}

	Shader::~Shader()
	{
	}

	const uint8_t* Shader::GetByteCode() const
	{
		return m_ByteCode.data();
	}

	size_t Shader::GetByteCodeSize() const
	{
		return m_ByteCode.size();
	}

	const vkr::Vector3u& Shader::GetNumThreads() const
	{
		return m_NumThreads;
	}

	vkr::Render::ShaderStage Shader::GetShaderStage() const
	{
		return m_Stage;
	}
}