#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	enum class ShaderModel
	{
		SM_6_0,
		SM_6_6,
		SM_6_7,
	};

	enum ShaderStage
	{
		SHADER_STAGE_VERTEX = 0,
		SHADER_STAGE_PIXEL = 1,
		SHADER_STAGE_COMPUTE = 2,
		//SHADER_STAGE_MESH = 3,
		//SHADER_STAGE_AMPLIFICATION = 4,

		SHADER_STAGE_COUNT
	};

	class Shader
	{
		friend class ShaderCompiler;
	public:
		Shader();
		~Shader();

		const uint8_t* GetByteCode() const;
		size_t GetByteCodeSize() const;

		const Vector3u& GetNumThreads() const;

		ShaderStage GetShaderStage() const;

	private:
		ShaderStage m_Stage;
		std::vector<uint8_t> m_ByteCode;

		// Reflection data
		Vector3u m_NumThreads;
	};
}