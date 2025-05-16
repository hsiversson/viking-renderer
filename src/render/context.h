#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	class Context
	{
	public:
		Context();
		~Context();

		void Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator);

		// Draw, DrawInstanced, Dispatch, Bind Resources etc..

	private:
		ComPtr<ID3D12GraphicsCommandList> m_CommandList;
		ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
		// ID3D12GraphicsCommandList...
	};
}