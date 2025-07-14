#pragma once
#include "core/types.h"

namespace vkr::Render
{
	class Buffer;
	class BufferView;
}

namespace vkr::Graphics
{
	class MaterialDataBuffer
	{
	public:
		MaterialDataBuffer();
		~MaterialDataBuffer();

		void Clear();
		void PrepareBuffer();

		uint32_t AddData(uint32_t size, const uint8_t* data);

		template<typename T>
		uint32_t AddData(const T& data)
		{
			return AddData(sizeof(T), reinterpret_cast<const uint8_t*>(&data));
		}

		const std::vector<uint8_t>& GetData() const;
		Render::Buffer* GetBuffer() const;

	private:
		std::vector<uint8_t> m_Data;
		Ref<Render::Buffer> m_Buffer;
		Ref<Render::BufferView> m_BufferView;
		bool m_RequireDataUpload;
	};
}