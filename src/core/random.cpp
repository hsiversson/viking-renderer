#include "random.h"

#include <random>

namespace vkr::Random
{
	struct RandomDevice
	{
		RandomDevice()
		{
			std::random_device rd;
			std::seed_seq seed{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
			rng.seed(seed);
		}

		std::mt19937 rng;

		std::uniform_int_distribution<uint32_t> u32_dist{ 0, UINT32_MAX };
		std::uniform_int_distribution<uint32_t> u16_dist{ 0, UINT16_MAX };
		std::uniform_int_distribution<uint32_t>  u8_dist{ 0, UINT8_MAX };

		std::uniform_real_distribution<float> f32_0_1_dist{ 0.0f, 1.0f };
		std::uniform_real_distribution<float> f32_neg1_1_dist{ -1.0f, 1.0f };
	};

	static RandomDevice& GetDevice()
	{
		static thread_local RandomDevice instance;
		return instance;
	}

	uint8_t Rnd_u8()
	{
		RandomDevice& device = GetDevice();
		return static_cast<uint8_t>(device.u8_dist(device.rng));
	}

	uint16_t Rnd_u16()
	{
		RandomDevice& device = GetDevice();
		return static_cast<uint16_t>(device.u16_dist(device.rng));
	}

	uint32_t Rnd_u32()
	{
		RandomDevice& device = GetDevice();
		return device.u32_dist(device.rng);
	}

	uint32_t Rnd_u32_range(uint32_t min, uint32_t max)
	{
		RandomDevice& device = GetDevice();
		std::uniform_int_distribution<uint32_t> dist(min, max);
		return dist(device.rng);
	}

	float Rnd_f32_0_1()
	{
		RandomDevice& device = GetDevice();
		return device.f32_0_1_dist(device.rng);
	}

	float Rnd_f32_neg1_1()
	{
		RandomDevice& device = GetDevice();
		return device.f32_neg1_1_dist(device.rng);
	}

	float Rnd_f32_range(float min, float max)
	{
		RandomDevice& device = GetDevice();
		std::uniform_real_distribution<float> dist(min, max);
		return dist(device.rng);
	}
}