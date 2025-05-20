#include "CommandLine.h"

namespace vkr
{
	CommandLine* CommandLine::g_Instance = nullptr;

	void CommandLine::Parse(int32_t argc, char** argv)
	{
		delete g_Instance;
		g_Instance = new CommandLine;

		std::string currentArg;
		for (int32_t i = 1; i < argc; ++i) 
		{
			std::string token = argv[i];
			if (token.starts_with("-")) 
			{
				currentArg = token.substr(1);
				g_Instance->m_Args.insert(currentArg);
			}
			else if (!currentArg.empty()) 
			{
				if (IsInt(token))
				{
					g_Instance->m_IntParams[currentArg].push_back(ToInt(token));
				}
				else if (IsFloat(token))
				{
					g_Instance->m_FloatParams[currentArg].push_back(ToFloat(token));
				}
				else
				{
					g_Instance->m_StringParams[currentArg].push_back(token);
				}
			}
		}
	}

	bool CommandLine::Has(const char* arg)
	{
		return g_Instance->m_Args.contains(arg);
	}

	uint32_t CommandLine::GetNumIntParams(const char* arg)
	{
		if (Has(arg))
		{
			return static_cast<uint32_t>(g_Instance->m_IntParams.at(arg).size());
		}
		else
			return 0;
	}

	int32_t CommandLine::GetIntParam(const char* arg, uint32_t paramIndex /*= 0*/)
	{
		const std::vector<int32_t>& intParams = g_Instance->m_IntParams.at(arg);
		assert(intParams.size() > paramIndex);
		return intParams.at(paramIndex);
	}

	uint32_t CommandLine::GetNumFloatParams(const char* arg)
	{
		if (Has(arg))
		{
			return static_cast<uint32_t>(g_Instance->m_FloatParams.at(arg).size());
		}
		else
			return 0;
	}

	float CommandLine::GetFloatParam(const char* arg, uint32_t paramIndex /*= 0*/)
	{
		const std::vector<float>& floatParams = g_Instance->m_FloatParams.at(arg);
		assert(floatParams.size() > paramIndex);
		return floatParams.at(paramIndex);
	}

	uint32_t CommandLine::GetNumStringParams(const char* arg)
	{
		if (Has(arg))
		{
			return static_cast<uint32_t>(g_Instance->m_StringParams.at(arg).size());
		}
		else
			return 0;
	}

	std::string CommandLine::GetStringParam(const char* arg, uint32_t paramIndex /*= 0*/)
	{
		const std::vector<std::string>& stringParams = g_Instance->m_StringParams.at(arg);
		assert(stringParams.size() > paramIndex);
		return stringParams.at(paramIndex);
	}
}