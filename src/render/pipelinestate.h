#pragma once

#include "d3d12header.h"

namespace vkr::Render
{
	class PipelineState
	{
	public:

	private:
		ComPtr<ID3D12PipelineState> m_PipelineState;
	};
}