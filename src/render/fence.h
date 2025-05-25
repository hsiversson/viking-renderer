#pragma once
#include "deviceobject.h"
#include "rendercommon.h"

namespace vkr::Render
{
	class Fence : public DeviceObject
	{
	public:
		Fence(Device& device);
		~Fence();

		uint64_t Increment();
		bool Wait(uint64_t value, bool block = true);
		bool IsPending(uint64_t value) const;

		ID3D12Fence* GetFence() const;
		uint64_t GetLastValue() const;

	private:
		ComPtr<ID3D12Fence> m_Fence;
		std::atomic<uint64_t> m_Value;
	};
}