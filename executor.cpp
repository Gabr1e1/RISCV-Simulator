//
// Created by Gabriel on 7/3/2019.
//

#include "executor.h"
#include "instruction.h"

Executor::Executor(int MemSize) : mem(new char[MemSize]), pc(0)
{
	memset(mem, 0, sizeof(mem));
}

void Executor::addInstruction(unsigned int offset, unsigned int inst)
{
//	std::cerr << offset << " " << inst << std::endl;
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
			curAdd = (unsigned int) Util::HEX2DEC(str, 1, 8);
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
				addInstruction(curAdd, (unsigned) Util::HEX2DEC(curStr, 0, (int) curStr.length() - 1));
				curAdd += 4;
				curStr = "";
			}
		}
	}
}

int Executor::execute()
{
	pc = 0;
	while (true)
	{
		auto curInst = *reinterpret_cast<unsigned int *>(mem + pc);
//		printf("PC: %x\n", pc);
		if (curInst == 0x00c68223) break;
		auto cur = parseInst(curInst);
		cur->IF(this);
		cur->ID(this);
		cur->EX(this);
		cur->MEM(this);
		cur->WB(this);
		reg[0] = 0;
		delete cur;
//		for (int i = 0; i < 32; i++) std::cout << reg[i] << " ";
//		std::cout << std::endl;
	}
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

