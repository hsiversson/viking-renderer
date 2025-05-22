#pragma once

namespace vkr
{
	//Fast hash function I found somewhere in the web. No idea if it works
	uint64_t hash_fnv64(const uint8_t* pBuffer, const uint8_t* const pBufferEnd)
	{
		const uint64_t MagicPrime = UINT64_C(0x00000100000001B3);
		uint64_t Hash = UINT64_C(0xCBF29CE484222325);

		for (; pBuffer < pBufferEnd; pBuffer++)
			Hash = (Hash ^ (*pBuffer)) * MagicPrime;

		return Hash;
	}
}