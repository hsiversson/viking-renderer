#include "context.h"

void vkr::Render::Context::Init(ID3D12GraphicsCommandList* commandList, ID3D12CommandAllocator* commandAllocator)
{
	m_CommandList = commandList;
	m_CommandAllocator = commandAllocator;
}

vkr::Render::Context::Context()
{

}

vkr::Render::Context::~Context()
{

}
