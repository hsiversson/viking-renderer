#pragma once
#include "render/rendercommon.h"

namespace vkr::Render
{
	struct TextureDesc;
	struct BufferDesc;

	DXGI_FORMAT D3DConvertFormat(Format format);
	Format D3DConvertFormat(DXGI_FORMAT format);

	D3D12_COMPARISON_FUNC D3DConvertComparisonFunc(ComparisonFunc comparisonFunc);
	ComparisonFunc D3DConvertComparisonFunc(D3D12_COMPARISON_FUNC comparisonFunc);

	D3D12_BLEND_OP D3DConvertBlendOp(BlendOp blendOp);
	BlendOp D3DConvertBlendOp(D3D12_BLEND_OP blendOp);

	D3D12_BLEND D3DConvertBlendArg(BlendArg blendArg);
	BlendArg D3DConvertBlendArg(D3D12_BLEND blendArg);

	D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DConvertPrimitiveType(PrimitiveType primitiveType);
	PrimitiveType D3DConvertPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType);

	D3D12_PRIMITIVE_TOPOLOGY D3DConvertPrimitiveTopology(PrimitiveTopology topologyType);
	PrimitiveTopology D3DConvertPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topologyType);

	D3D12_BARRIER_ACCESS D3DConvertResourceStateAccess(ResourceStateAccess access);
	ResourceStateAccess D3DConvertResourceStateAccess(D3D12_BARRIER_ACCESS access);

	D3D12_BARRIER_SYNC D3DConvertResourceStateSync(ResourceStateSync sync);
	ResourceStateSync D3DConvertResourceStateSync(D3D12_BARRIER_SYNC sync);

	D3D12_BARRIER_LAYOUT D3DConvertResourceStateLayout(ResourceStateLayout layout);
	ResourceStateLayout D3DConvertResourceStateLayout(D3D12_BARRIER_LAYOUT layout);

	D3D12_RESOURCE_DESC1 D3DConvertTextureDesc(const TextureDesc& desc);
	D3D12_RESOURCE_DESC1 D3DConvertBufferDesc(const BufferDesc& desc);

	D3D12_HEAP_PROPERTIES D3DGetDefaultHeapProperties();
	D3D12_HEAP_PROPERTIES D3DGetUploadHeapProperties();
}