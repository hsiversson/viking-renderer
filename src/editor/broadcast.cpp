#include "broadcast.h"
#include "editor.h"

namespace vkr::Editor
{
	BroadcastMessage::BroadcastMessage(const BroadcastMessageId id)
		: m_Id(id)
	{
	}

	void BroadcastMessage::SetData(uint32_t size, const uint8_t* data)
	{
		m_Data.resize(size);
		memcpy(m_Data.data(), data, size);
	}

	const uint8_t* BroadcastMessage::GetData() const
	{
		return m_Data.data();
	}

	uint32_t BroadcastMessage::GetDataSize() const
	{
		return m_Data.size();
	}

	BroadcastMessageId BroadcastMessage::GetId() const
	{
		return m_Id;
	}

	BroadcastListener::BroadcastListener()
	{
		if (Manager* manager = Manager::Get())
			manager->RegisterBroadcastListener(this);
	}

	BroadcastListener::~BroadcastListener()
	{
		if (Manager* manager = Manager::Get())
			manager->UnregisterBroadcastListener(this);
	}
}