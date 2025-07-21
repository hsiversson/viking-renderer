#pragma once

namespace vkr
{
	using ReturnCode = int32_t;
	enum ReturnCodes : int32_t 
	{
		RETURN_OK,
		RETURN_ERROR,
		RETURN_INVALID_ARG,
	};

	template<typename T, uint32_t N>
	class MovingAverage 
	{
	public:
		MovingAverage() 
			: m_Index(0)
			, m_Count(0)
			, m_Sum(0) 
		{
			m_Buffer.fill(T{});
		}

		void Add(T value) 
		{
			m_Sum -= m_Buffer[m_Index];
			m_Buffer[m_Index] = value;
			m_Sum += value;

			m_Index = (m_Index + 1) % N;
			m_Count = std::min(m_Count + 1, N);
		}

		T GetAverage() const 
		{
			return m_Count > 0 ? m_Sum / static_cast<T>(m_Count) : T{};
		}

		void Reset() 
		{
			m_Buffer.fill(T{});
			m_Index = 0;
			m_Count = 0;
			m_Sum = T{};
		}

	private:
		std::array<T, N> m_Buffer;
		uint32_t m_Index;
		uint32_t m_Count;
		T m_Sum;
	};
}

#include "vector.h"
#include "matrix.h"