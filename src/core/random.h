#pragma once
#include <cstdint>

namespace vkr::Random
{
	uint8_t  Rnd_u8();									// Returns a uint8_t with a random value in the range [0-255]
	uint16_t Rnd_u16();									// Returns a uint16_t with a random value in the range [0-65535]
	uint32_t Rnd_u32();									// Returns a uint32_t with a random value in the range [0-4294967295]
	uint32_t Rnd_u32_range(uint32_t min, uint32_t max); // Returns a uint32_t with a random value in the range [min-max]

	float Rnd_f32_0_1();								// Returns a float with a random value in the range [0-1]
	float Rnd_f32_neg1_1();								// Returns a float with a random value in the range [-1-1]
	float Rnd_f32_range(float min, float max);			// Returns a float with a random value in the range [min-max]
}
