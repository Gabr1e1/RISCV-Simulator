//
// Created by Gabriel on 7/2/2019.
//

#ifndef RISCV_SIMULATOR_UTILITY_HPP
#define RISCV_SIMULATOR_UTILITY_HPP

#include <algorithm>

class Util
{
public:
	inline static unsigned int bitmask(unsigned int l, unsigned int r)
	{
		if (l > r) std::swap(l, r);
		return (1u << r) - (1u << l);
	}
	inline static unsigned int getBits(unsigned int l, unsigned int r, unsigned int a)
	{
		return (a & bitmask(l,r)) >> l;
	}
};

#endif //RISCV_SIMULATOR_UTILITY_HPP
