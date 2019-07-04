//
// Created by Gabriel on 7/3/2019.
//

#ifndef RISCV_SIMULATOR_EXECUTOR_H
#define RISCV_SIMULATOR_EXECUTOR_H

#include <iostream>
#include <cstring>
#include "utility.hpp"

class Instruction;

class Executor
{
public:
	int reg[32] = {0};
	char *mem;

	int pipelineRegister[4][5] = {{0}};
	int pc = 0;

public:
	Executor(int MemSize = 0x20000);

	void addInstruction(unsigned int offset, unsigned int inst);

	void read();

	int execute();

	Instruction *parseInst(unsigned int inst);
};

#endif //RISCV_SIMULATOR_EXECUTOR_H
