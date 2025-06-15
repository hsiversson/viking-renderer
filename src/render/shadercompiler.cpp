#include "shadercompiler.h"

#include <dxcapi.h>

namespace vkr::Render
{
	static const wchar_t* GetShaderModelString(ShaderModel shaderModel)
	{
		switch (shaderModel)
		{
		case ShaderModel::SM_6_0:
			return L"6_0";
		default:
		case ShaderModel::SM_6_6:
			return L"6_6";
		case ShaderModel::SM_6_7:
			return L"6_7";
		}
	}

	static const wchar_t* GetShaderStageString(ShaderStage shaderStage)
	{
		switch (shaderStage)
		{
		case SHADER_STAGE_VERTEX:
			return L"vs";
		case SHADER_STAGE_PIXEL:
			return L"ps";
		case SHADER_STAGE_COMPUTE:
			return L"cs";
		//case SHADER_STAGE_MESH:
		//	return L"ms";
		//case SHADER_STAGE_AMPLIFICATION:
		//	return L"as";
		default:
			// error
			return nullptr;
		}
	}

	static std::wstring GetTargetProfile(ShaderStage shaderStage, ShaderModel shaderModel)
	{
		std::wstring target;
		target.reserve(8);
		target.append(GetShaderStageString(shaderStage));
		target.append(L"_");
		target.append(GetShaderModelString(shaderModel));
		return target;
	}

	ShaderCompiler::ShaderCompiler()
	{
		DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(m_Compiler.GetAddressOf()));
		DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_Utils.GetAddressOf()));
		m_Utils->CreateDefaultIncludeHandler(m_IncludeHandler.GetAddressOf());
	}

	ShaderCompiler::~ShaderCompiler()
	{

	}

	bool ShaderCompiler::CompileFromFile(Shader& outShader, const std::filesystem::path& filepath, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel /*= ShaderModel::SM_6_6*/)
	{
		std::string shaderSource = ReadFileToString(filepath);
		return CompileFromMemory(outShader, shaderSource, entryPoint, stage, shaderModel);
	}

	bool ShaderCompiler::CompileFromMemory(Shader& outShader, const std::string& shaderSource, const wchar_t* entryPoint, ShaderStage stage, ShaderModel shaderModel /*= ShaderModel::SM_6_6*/)
	{
		ComPtr<IDxcBlobEncoding> sourceBlob;
		HRESULT hr = m_Utils->CreateBlob((LPBYTE)shaderSource.data(), static_cast<UINT32>(shaderSource.size()), DXC_CP_UTF8, sourceBlob.GetAddressOf());
		if (FAILED(hr)) 
		{
			// Create blob failed
			return false;
		}

		std::vector<LPCWSTR> compileArguments;
		compileArguments.push_back(L"-E");
		compileArguments.push_back(entryPoint);

		const std::wstring targetProfile = GetTargetProfile(stage, shaderModel);
		compileArguments.push_back(L"-T");
		compileArguments.push_back(targetProfile.c_str());

		DxcBuffer sourceBuffer;
		sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
		sourceBuffer.Size = sourceBlob->GetBufferSize();
		sourceBuffer.Encoding = 1;

		ComPtr<IDxcResult> result;
		hr = m_Compiler->Compile(&sourceBuffer, compileArguments.data(), compileArguments.size(), m_IncludeHandler.Get(), IID_PPV_ARGS(&result));
		if (FAILED(hr)) 
		{
			// Compile call failed
			return false;
		}

		if (result->HasOutput(DXC_OUT_ERRORS))
		{
			ComPtr<IDxcBlobUtf8> errors;
			ComPtr<IDxcBlobUtf16> errorsOutputName;
			hr = result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), &errorsOutputName);
			if (errors && errors->GetStringLength() > 0)
			{
				OutputDebugString(errors->GetStringPointer());
				return false;
			}
		}

		if (result->HasOutput(DXC_OUT_OBJECT))
		{
			ComPtr<IDxcBlob> shaderByteCode;
			hr = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderByteCode), nullptr);
			if (SUCCEEDED(hr))
			{
				outShader.m_ByteCode.resize(shaderByteCode->GetBufferSize());
				memcpy(outShader.m_ByteCode.data(), shaderByteCode->GetBufferPointer(), shaderByteCode->GetBufferSize());
				outShader.m_Stage = stage;
			}
			else
			{
				// assert(false)
				return false;
			}
		}
		else
		{
			// assert(false)
			return false;
		}

		// reflection data?
		if (result->HasOutput(DXC_OUT_REFLECTION))
		{
			ComPtr<IDxcBlob> reflectionData;
			hr = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(&reflectionData), nullptr);
			if (SUCCEEDED(hr))
			{
				DxcBuffer reflectionBuffer;
				reflectionBuffer.Ptr = reflectionData->GetBufferPointer();
				reflectionBuffer.Size = reflectionData->GetBufferSize();
				reflectionBuffer.Encoding = 1;

				ComPtr<ID3D12ShaderReflection> reflection;
				if (SUCCEEDED(m_Utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(&reflection))))
				{
					D3D12_SHADER_DESC shaderDesc = {};
					reflection->GetDesc(&shaderDesc);

					uint32_t numThreads[3];
					reflection->GetThreadGroupSize(&numThreads[0], &numThreads[1], &numThreads[2]);
					outShader.m_NumThreads = Vector3u{ numThreads[0], numThreads[1], numThreads[2] };
				}
			}
		}
		
		// pdb output?

		return true;
	}
}