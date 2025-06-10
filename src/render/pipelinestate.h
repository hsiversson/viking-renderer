#pragma once

#include "render/rendercommon.h"
#include "render/shader.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class RootSignature;
	class Device;

	enum PipelineStateType
	{
		PIPELINE_STATE_TYPE_DEFAULT,
		PIPELINE_STATE_TYPE_COMPUTE,
		//PIPELINE_STATE_TYPE_MESH,

		PIPELINE_STATE_TYPE_COUNT,
		PIPELINE_STATE_TYPE_UNKNOWN = PIPELINE_STATE_TYPE_COUNT
	};

	struct DefaultPipelineStateDesc
	{
		Shader* m_VertexShader;
		Shader* m_PixelShader;

		PrimitiveType m_PrimitiveType;
		VertexLayout m_VertexLayout;
		RasterizerState m_RasterizerState;
		DepthStencilState m_DepthStencilState;
		RenderTargetState m_RenderTargetState;
		BlendState m_BlendState;
	};

	struct ComputePipelineStateDesc
	{
		Shader* m_ComputeShader;
	};

	//struct MeshPipelineStateDesc
	//{
	//	Shader* m_MeshShader;
	//  Shader* m_AmplificationShader;
	//	Shader* m_PixelShader;
	//
	//	VertexLayout m_VertexLayout;
	//	RasterizerState m_RasterizerState;
	//	DepthStencilState m_DepthStencilState;
	//	RenderTargetState m_RenderTargetState;
	//	BlendState m_BlendState;
	//};

	struct PipelineStateDesc
	{
		PipelineStateType m_Type;
		union
		{
			DefaultPipelineStateDesc Default;
			ComputePipelineStateDesc Compute;
			//MeshPipelineStateDesc Mesh;
		};

		PipelineStateDesc() : m_Type(PIPELINE_STATE_TYPE_UNKNOWN), Default{} {}
		~PipelineStateDesc() {}
	};

	struct PipelineStateDefaultMetaData
	{
		// something?
	};

	struct PipelineStateComputeMetaData
	{
		Vector3u m_NumThreads;
	};

	struct PipelineStateMetaData
	{
		PipelineStateType m_Type;
		union
		{
			PipelineStateDefaultMetaData Default;
			PipelineStateComputeMetaData Compute;
		};

		PipelineStateMetaData() : m_Type(PIPELINE_STATE_TYPE_UNKNOWN), Default{} {}
		~PipelineStateMetaData() {}
	};

	class PipelineState
	{
	public:
		PipelineState();
		~PipelineState();

		bool Init(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature);

		ID3D12PipelineState* GetD3DPipelineState() const;
		const Ref<RootSignature>& GetRootSignature() const;
		const PipelineStateMetaData& GetMetaData() const;
	private:
		ComPtr<ID3D12PipelineState> m_PipelineState;
		Ref<RootSignature> m_RootSignature;
		PipelineStateMetaData m_MetaData;
	};
}