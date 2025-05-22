#pragma once

#include "render/deviceobject.h"
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

		PIPELINE_STATE_TYPE_COUNT
	};

	struct DefaultPipelineStateDesc
	{
		Shader* m_VertexShader;
		Shader* m_PixelShader;

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
	};

	class PipelineState : public DeviceObject
	{
	public:
		PipelineState(Device& device);
		~PipelineState();

		bool Init(const PipelineStateDesc& desc, RootSignature* rootSignature);

		ID3D12PipelineState* GetD3DPipelineState() const;
		RootSignature* GetRootSignature() const;
	private:
		ComPtr<ID3D12PipelineState> m_PipelineState;

		RootSignature* m_RootSignature;
	};
}