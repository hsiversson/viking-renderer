#pragma once
#include "core/types.h"

namespace vkr
{
	class CommandLine
	{
	public:
		static void Parse(int32_t argc, char** argv);

		static bool Has(const char* arg);

		static uint32_t GetNumIntParams(const char* arg);
		static int32_t GetIntParam(const char* arg, uint32_t paramIndex = 0);

		static uint32_t GetNumFloatParams(const char* arg);
		static float GetFloatParam(const char* arg, uint32_t paramIndex = 0);

		static uint32_t GetNumStringParams(const char* arg);
		static std::string GetStringParam(const char* arg, uint32_t paramIndex = 0);

	private:
		std::unordered_set<std::string> m_Args;
		std::unordered_map<std::string, std::vector<int32_t>> m_IntParams;
		std::unordered_map<std::string, std::vector<float>> m_FloatParams;
		std::unordered_map<std::string, std::vector<std::string>> m_StringParams;

		static CommandLine* g_Instance;
	};
}