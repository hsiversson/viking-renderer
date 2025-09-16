#pragma once

namespace vkr::Editor
{
	using BroadcastMessageId = uint64_t;
	class BroadcastMessage
	{
	public:
		BroadcastMessage(const BroadcastMessageId id);

		void SetData(uint32_t size, const uint8_t* data);
		const uint8_t* GetData() const;
		uint32_t GetDataSize() const;

		template<typename T>
		void SetData(const T& data)
		{
			SetData(sizeof(T), reinterpret_cast<const uint8_t*>(&data));
		}

		template<typename T>
		void GetData(T& data) const
		{
			memcpy(&data, GetData(), GetDataSize());
		}

		BroadcastMessageId GetId() const;

	private:
		const BroadcastMessageId m_Id;
		std::vector<uint8_t> m_Data;
	};

	class BroadcastListener
	{
		friend class Manager;
	public:
		BroadcastListener();
		~BroadcastListener();

	protected:
		virtual void ReceiveMessage(const BroadcastMessage& aMessage) = 0;
	};

	static constexpr BroadcastMessageId BROADCAST_MSG_ID_SELECTED_ENTITIES = 100;
}