#pragma once

namespace vkr::Render
{
	class Device;
	class DeviceObject
	{
	public:
		DeviceObject(Device& device) : m_Device(device) {}
		Device& GetDevice() { return m_Device; }
	protected:
		Device& m_Device;
	};
}