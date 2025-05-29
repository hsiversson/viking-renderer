#pragma once
#include "render/rendercommon.h"

namespace vkr::Render
{
	DXGI_FORMAT D3DConvertFormat(Format format);
	Format D3DConvertFormat(DXGI_FORMAT format);

	D3D12_COMPARISON_FUNC D3DConvertComparisonFunc(ComparisonFunc comparisonFunc);
	ComparisonFunc D3DConvertComparisonFunc(D3D12_COMPARISON_FUNC comparisonFunc);

	D3D12_PRIMITIVE_TOPOLOGY_TYPE D3DConvertPrimitiveType(PrimitiveType primitiveType);
	PrimitiveType D3DConvertPrimitiveType(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveType);
}