//
// Created by Gabriel on 7/3/2019.
//

#include "executor.h"
#include "instruction.h"

Executor::Executor(int MemSize) : mem(new char[MemSize]), corres(new Instruction *[MemSize])
{
	memset(mem, 0, sizeof(mem));
	memset(corres, 0, sizeof(corres));
}

Executor::~Executor()
{
	delete[] mem;
	delete[] corres;
}

void Executor::addInstruction(unsigned int offset, unsigned int inst)
{
	memcpy(mem + offset, reinterpret_cast<char *>(&inst), sizeof(inst));
}

void Executor::read()
{
	unsigned int curAdd = 0;
	int cnt = 0;
	std::string str, curStr = "";

	while (std::cin >> str)
	{
		if (str[0] == '@')
		{
			curAdd = Util::HEX2DEC(str, 1, 8);
			curStr = "";
		}
		else
		{
			cnt = (cnt + 1) % 4;
			curStr = str + curStr;
			if (cnt == 0)
			{
				addInstruction(curAdd, Util::HEX2DEC(curStr, 0, (int) curStr.length() - 1));
				curAdd += 4;
				curStr = "";
			}
		}
	}
}

bool Executor::lockCheck()
{
	auto a = Util::getBits(15, 19, (unsigned) pipelineRegister[0][IR0]);
	auto b = Util::getBits(20, 24, (unsigned) pipelineRegister[0][IR0]);

	auto op = pipelineRegister[1][IR1] & 0b1111111;
	auto c = Util::getBits(7, 11, (unsigned) pipelineRegister[1][IR1]);
	return (op == 0b0000011) && (a == c || b == c);
}

int Executor::execute()
{
	pc = 0;
	Instruction *cur[5] = {nullptr};
	unsigned int curInst = 0;

	do
	{
		if (cur[4] != nullptr) cur[4]->WB(this), cur[4] = nullptr;
		reg[0] = 0;
		if (cur[3] != nullptr) cur[3]->MEM(this);
		if (cur[2] != nullptr) cur[2]->EX(this);
		if (cur[1] != nullptr) cur[1]->ID(this);

		if (cur[0] == nullptr) //no command stalling
		{
			curInst = *reinterpret_cast<unsigned int *>(mem + pc);
			cur[0] = parseInst(curInst, pc);

			if (cur[0] != nullptr && !(cur[0]->IF(this)))
			{
				cur[0] = nullptr;
				cur[1] = nullptr;
			}

			bool t = (cur[0] != nullptr) && lockCheck();
			for (int i = 4; i > 0 + t; i--) cur[i] = cur[i - 1];
			cur[0 + t] = nullptr;
		}
		else
		{
			for (int i = 4; i > 0; i--) cur[i] = cur[i - 1];
			cur[0] = nullptr;
		}
	} while ((cur[0] != nullptr || cur[1] != nullptr || cur[2] != nullptr || cur[3] != nullptr || cur[4] != nullptr) ||
			 (curInst != 0x00c68223));

	return ((unsigned int) reg[10]) & 255u;
}

Instruction *Executor::parseInst(unsigned int inst, int loc)
{
	if (inst == 0x00c68223) return nullptr;
	if (corres[loc] != nullptr) return corres[loc];

	auto op = inst & 0b1111111;
	switch (op)
	{
		case 0b0110111: //LUI
			return corres[loc] = new IntCom(inst, U);
		case 0b0010111: //AUIPC
			return corres[loc] = new IntCom(inst, U);
		case 0b0110011:
			return corres[loc] = new IntCom(inst, I);
		case 0b0010011:
			return corres[loc] = new IntCom(inst, I);
		case 0b0000011:
			return corres[loc] = new LoadNStore(inst, I);
		case 0b0100011:
			return corres[loc] = new LoadNStore(inst, S);
		case 0b1101111: //JAL
			return corres[loc] = new CtrlTrans(inst, J);
		case 0b1100111: //JALR
			return corres[loc] = new CtrlTrans(inst, I);
		case 0b1100011:
			return corres[loc] = new CtrlTrans(inst, B);
		default:
//			std::cerr << "CANT PARSE: " << inst << std::endl;
			return nullptr;
	}
}

