//
// Created by Gabriel on 7/2/2019.
//

#ifndef RISCV_SIMULATOR_UTILITY_HPP
#define RISCV_SIMULATOR_UTILITY_HPP

#include <algorithm>
#include <string>

class Util
{
public:
	static unsigned int bitmask(unsigned int l, unsigned int r)
	{
		return (unsigned int)((1ull << (r + 1)) - (1ull << l));
	}

	static unsigned int getBits(unsigned int l, unsigned int r, unsigned int a)
	{
		return (a & (unsigned int)((1ull << (r + 1)) - (1ull << l))) >> l;
	}

	static void writeBits(unsigned int l, unsigned int r, int &a, int x)
	{
		a = (a & (0xffffffff ^ bitmask(l,r))) | (x << l);
	}

	static unsigned int HEX2DEC(const std::string &str, int l, int r)
	{
		unsigned int ret = 0;
		for (int i = l; i <= r; i++)
			ret = ret * 16 + ((str[i] >= '0' && str[i] <= '9') ? (str[i] - '0') : (str[i] - 'A' + 10));
		return ret;
	}
};

#endif //RISCV_SIMULATOR_UTILITY_HPP
