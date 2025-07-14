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

	const Vector3u& Shader::GetNumThreads() const
	{
		return m_NumThreads;
	}

	ShaderStage Shader::GetShaderStage() const
	{
		return m_Stage;
	}

	Ref<Shader> ShaderCache::Get(uint64_t hash) const
	{
		if (!m_Cache.contains(hash))
			return nullptr;

		return m_Cache.at(hash);
	}

	void ShaderCache::Insert(uint64_t hash, const Ref<Shader>& shader)
	{
		m_Cache[hash] = shader;
	}

}