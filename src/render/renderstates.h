#pragma once

#include "render/rendercommon.h"

namespace vkr::Render
{
	struct VertexLayout
	{
		std::vector<D3D12_INPUT_ELEMENT_DESC> m_D3DElements;
	};

	struct RasterizerState
	{
		D3D12_RASTERIZER_DESC m_D3DRasterizerState;
	};

	struct DepthStencilState
	{
		D3D12_DEPTH_STENCIL_DESC m_D3DDepthStencilState;
	};

	struct RenderTargetState
	{
		std::vector<DXGI_FORMAT> m_D3DRTFormats;
	};

	struct BlendState
	{
		D3D12_BLEND_DESC m_D3DBlendDesc;
	};
}