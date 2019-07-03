//
// Created by Gabriel on 7/2/2019.
//

#ifndef RISCV_SIMULATOR_EXECUTOR_HPP
#define RISCV_SIMULATOR_EXECUTOR_HPP

#include <cstring>
#include <string>
#include "instruction.hpp"

class Executor
{
	friend Instruction;
	friend CtrlTrans;
	friend LoadNStore;
	friend IntCom;

private:
	int reg[32] = {0}, pc;
	char *mem;

public:
	Executor(int MemSize = 0x20000) : mem(new char[MemSize]), pc(0)
	{
		memset(mem, 0, sizeof(mem));
	}

public:
	void addInstruction(unsigned int offset, unsigned int inst)
	{
		memcpy(mem + offset, reinterpret_cast<char *>(&inst), sizeof(inst));
	}

public:
	void read()
	{
		unsigned int curAdd = 0;
		std::string str, curStr = "";

		while (std::cin >> str)
		{
			if (str[0] == '@')
			{
				addInstruction(curAdd, (unsigned int)Util::HEX2DEC(curStr, 0, (int)curStr.length() - 1));
				curAdd = (unsigned int)Util::HEX2DEC(str,1,8);
				curStr = "";
			}
			else curStr = str + curStr;
		}
	}

	int execute()
	{
		pc = 0;
		while (true)
		{
			auto curInst = *reinterpret_cast<unsigned int*>(mem + pc);
			if (curInst == 0x00c68223) break;
			auto cur = InstructionParser::parseInst(curInst);
			cur->IF(this);
			cur->ID(this);
			cur->EX(this);
			cur->MEM(this);
			cur->WB(this);
		}
		return ((unsigned int)reg[10]) & 255u;
	}

};

#endif //RISCV_SIMULATOR_EXECUTOR_HPP
