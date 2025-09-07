#pragma once
#include "rendercommon.h"

namespace vkr::Render
{
	class FenceResource 
	{
	public:
		FenceResource();
		~FenceResource();

		uint64_t Increment();
		bool Wait(uint64_t value, bool block = true);
		bool IsPending(uint64_t value) const;

		ID3D12Fence* GetFence() const;
		uint64_t GetNextValue() const;
		uint64_t GetLastValue() const;

	private:
		ComPtr<ID3D12Fence> m_Fence;
		std::atomic<uint64_t> m_Value;
	};

	struct Fence
	{
		uint64_t m_Value;
		FenceResource* m_FenceResource;

		Fence();
		Fence(FenceResource* fence, uint64_t value);

		Fence operator+(uint64_t v) const;
		Fence& operator+=(uint64_t v);

		bool Wait(bool block = true);
		bool IsPending() const;
	};
}