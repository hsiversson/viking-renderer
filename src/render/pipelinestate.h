#pragma once

#include "render/rendercommon.h"
#include "render/shader.h"
#include "render/renderstates.h"

namespace vkr::Render
{
	class RootSignature;
	class Buffer;
	class Device;

	enum PipelineStateType
	{
		PIPELINE_STATE_TYPE_DEFAULT,
		PIPELINE_STATE_TYPE_COMPUTE,
		//PIPELINE_STATE_TYPE_MESH,
		PIPELINE_STATE_TYPE_RAYTRACING,

		PIPELINE_STATE_TYPE_COUNT,
		PIPELINE_STATE_TYPE_UNKNOWN = PIPELINE_STATE_TYPE_COUNT
	};

	struct DefaultPipelineStateDesc
	{
		Shader* m_VertexShader = nullptr;
		Shader* m_PixelShader = nullptr;

		PrimitiveType m_PrimitiveType;
		VertexLayout m_VertexLayout;
		RasterizerState m_RasterizerState;
		DepthStencilState m_DepthStencilState;
		RenderTargetState m_RenderTargetState;
		BlendState m_BlendState;
	};

	struct ComputePipelineStateDesc
	{
		Shader* m_ComputeShader = nullptr;
	};

	struct RaytracingHitGroupDesc
	{
		Shader* m_Shader = nullptr;
		std::wstring m_Identifier;
		std::wstring m_ClosestHitIdentifier;
		std::wstring m_AnyHitIdentifier;
	};

	struct RaytracingPipelineStateDesc
	{
		Shader* m_Shader = nullptr;
		std::wstring m_RayGenerationIdentifier;
		std::wstring m_MissIdentifier;

		std::vector<RaytracingHitGroupDesc> m_HitGroups;
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
			RaytracingPipelineStateDesc Raytracing;
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

	struct PipelineStateRaytracingMetaData
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
			PipelineStateComputeMetaData Raytracing;
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
		ID3D12StateObject* GetD3DStateObject() const;
		const D3D12_DISPATCH_RAYS_DESC& GetD3DDispatchRaysDesc() const;
		const Ref<RootSignature>& GetRootSignature() const;
		const PipelineStateMetaData& GetMetaData() const;
		PipelineStateType GetType() const;

	private:
		bool InitDefault(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature);
		bool InitCompute(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature);
		bool InitRaytracing(const PipelineStateDesc& desc, Ref<RootSignature> rootSignature);

		ComPtr<ID3D12PipelineState> m_PipelineState;
		Ref<RootSignature> m_RootSignature;
		PipelineStateMetaData m_MetaData;

		ComPtr<ID3D12StateObject> m_StateObject;
		Ref<Buffer> m_RaytracingShaderTable;
		D3D12_DISPATCH_RAYS_DESC m_RaytracingDispatchDesc;
	};
}