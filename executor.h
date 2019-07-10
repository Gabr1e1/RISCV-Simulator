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
	static const int BHT_ENTRY = 4096;
public:
	int reg[32] = {0};
	char *mem;

	int pipelineRegister[4][6] = {{0}};
	int pc = 0;
	Instruction **corres;
	int history[BHT_ENTRY / 16], pattern[BHT_ENTRY / 4];


public:
	int total = 0, miss = 0;

public:
	Executor(int MemSize = 0x20000);
	~Executor();

	void addInstruction(unsigned int offset, unsigned int inst);

	void read();

	void clearRegister(int x);

	int execute();

	Instruction *parseInst(unsigned int inst, int loc);

	bool lockCheck();
};

#endif //RISCV_SIMULATOR_EXECUTOR_H
