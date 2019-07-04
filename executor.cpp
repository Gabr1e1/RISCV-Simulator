//
// Created by Gabriel on 7/3/2019.
//

#include "executor.h"
#include "instruction.h"

Executor::Executor(int MemSize) : mem(new char[MemSize])
{
	memset(mem, 0, sizeof(mem));
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
//				std::cerr << curAdd << " " << curStr << " "
//						  << (unsigned) Util::HEX2DEC(curStr, 0, (int) curStr.length() - 1) << std::endl;
				addInstruction(curAdd, Util::HEX2DEC(curStr, 0, (int) curStr.length() - 1));
				curAdd += 4;
				curStr = "";
			}
		}
	}
}

int Executor::execute()
{
	pc = 0;
	Instruction *cur[5] = {nullptr};
	do
	{
		printf("%x\n",pc);
		if (cur[4] != nullptr) cur[4]->WB(this), delete cur[4];
		if (cur[3] != nullptr) cur[3]->MEM(this);
		if (cur[2] != nullptr) cur[2]->EX(this);
		if (cur[1] != nullptr) cur[1]->ID(this);
		for (int i = 4; i >= 0; i--) cur[i] = cur[i - 1];
		auto curInst = *reinterpret_cast<unsigned int *>(mem + pc);
		if (curInst == 0x00c68223) continue;
		cur[0] = parseInst(curInst);
		cur[0]->IF(this);
	} while (cur[0] != nullptr || cur[1] != nullptr || cur[2] != nullptr || cur[3] != nullptr || cur[4] != nullptr);

	return ((unsigned int) reg[10]) & 255u;
}

Instruction *Executor::parseInst(unsigned int inst)
{
	auto op = Util::getBits(0, 6, inst);
	switch (op)
	{
		case 0b0110111: //LUI
			return new IntCom(inst, U);
		case 0b0010111: //AUIPC
			return new IntCom(inst, U);
		case 0b0110011:
			return new IntCom(inst, I);
		case 0b0010011:
			return new IntCom(inst, I);
		case 0b0000011:
			return new LoadNStore(inst, I);
		case 0b0100011:
			return new LoadNStore(inst, S);
		case 0b1101111: //JAL
			return new CtrlTrans(inst, J);
		case 0b1100111: //JALR
			return new CtrlTrans(inst, I);
		case 0b1100011:
			return new CtrlTrans(inst, B);
		default:
//			std::cerr << "CANT PARSE: " << inst << std::endl;
			break;
	}
}

