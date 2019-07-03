//
// Created by Gabriel on 7/2/2019.
//

#ifndef RISCV_SIMULATOR_EXECUTOR_HPP
#define RISCV_SIMULATOR_EXECUTOR_HPP

#include <cstring>
#include "instruction.hpp"

class Executor
{
	friend Instruction;
private:
	int reg[32], pc;
	char *mem;

public:
	Executor(int MemSize = 0x20000) : mem(new char[MemSize])
	{
		memset(mem, 0, sizeof(mem));
	}

public:
	void addInstruction(unsigned int offset, unsigned int inst)
	{
		memcpy(mem + offset, reinterpret_cast<char *>(&inst), sizeof(inst));
	}

public:
	void execute()
	{
		pc = 0;
		
	}


};

#endif //RISCV_SIMULATOR_EXECUTOR_HPP
