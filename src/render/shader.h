#pragma once
#include "render/rendercommon.h"

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
		SHADER_STAGE_VERTEX,
		SHADER_STAGE_PIXEL,
		SHADER_STAGE_COMPUTE,
		//SHADER_STAGE_MESH,
		//SHADER_STAGE_AMPLIFICATION,
		SHADER_STAGE_RAYTRACING,

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

	class ShaderCache
	{
	public:
		Ref<Shader> Get(uint64_t hash) const;
		void Insert(uint64_t hash, const Ref<Shader>& shader);

	private:
		std::unordered_map<uint64_t, Ref<Shader>> m_Cache;
	};
}