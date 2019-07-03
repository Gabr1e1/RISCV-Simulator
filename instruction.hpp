//
// Created by Gabriel on 7/2/2019.
//

#ifndef RISCV_SIMULATOR_INSTRUCTION_HPP
#define RISCV_SIMULATOR_INSTRUCTION_HPP

#include <iostream>
#include "utility.hpp"
#include "executor.hpp"

enum EncodingType { R, I, S, B, U, J };
enum InstructionType { CT, LNS, IC };

class Instruction
{
public:
	InstructionType typeInst;
	EncodingType typeEnc;

public:
	unsigned int inst;
	unsigned int rs1, rs2, rd;
	int rs1v, rs2v;
	int imm;

public:
	Instruction(unsigned int _inst, EncodingType _typeEnc) : inst(_inst), typeEnc(_typeEnc)
	{}

public:
	void IF(Executor *exec)
	{
		exec->pc += 4;
	}

	void ID(Executor *exec)
	{
		rs1 = Util::getBits(15, 19, inst);
		rs1v = exec->reg[rs1];

		rs2 = Util::getBits(20, 24, inst);
		rs2v = exec->reg[rs2];

		rd = Util::getBits(7, 11, inst);
		switch (typeEnc)
		{
			case I:
				imm = Util::getBits(20, 20, inst) | (Util::getBits(21, 24, inst) << 1) |
					  (Util::getBits(25, 30, inst) << 5) |
					  ((Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0) << 11);
				break;
			case S:
				imm = Util::getBits(7, 7, inst) | (Util::getBits(8, 11, inst) << 1) |
					  (Util::getBits(25, 30, inst) << 5) |
					  ((Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0) << 11);
				break;
			case B:
				imm = (Util::getBits(8, 11, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
					  (Util::getBits(7, 7, inst) << 11) |
					  ((Util::getBits(31, 31, inst) ? Util::bitmask(12, 31) : 0) << 12);
				break;
			case U:
				imm = (Util::getBits(12, 19, inst) << 12) | (Util::getBits(20, 31, inst) << 20);
				break;
			case J:
				imm = (Util::getBits(21, 24, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
					  (Util::getBits(20, 20, inst) << 11) | (Util::getBits(12, 19, inst) << 12) |
					  ((Util::getBits(31, 31, inst) ? Util::bitmask(12, 31) : 0) << 12);
				break;
			default:
				imm = 0;
				break;
		}
	}

	virtual void EX(Executor *exec) = delete;

	virtual void MEM(Executor *exec) = delete;

	virtual void WB(Executor *exec) = delete;
};

class CtrlTrans : Instruction
{
private:
	enum CTType
	{
		BEQ, BNE, XXX, XXXX, BLT, BGE, BLTU, BGEU,
		JAL, JALR
	};
	CTType type;

public:
	CtrlTrans(unsigned int _inst, EncodingType _type) : Instruction(inst, _type)
	{
		typeInst = CT;
		auto op = Util::getBits(0, 6, inst);
		if (op == 0b1100011) type = (CTType)Util::getBits(12,14,inst);
		else if (op == 0b1101111) type = JAL;
		else type = JALR;
	}

	void EX(Executor *exec)
	{

	}
};

class LoadNStore : Instruction
{
private:
	enum LNSType
	{
		LB, LH, LW, XXX, LBU, LHU,
		SB, SH, SW
	};
	LNSType type;

public:
	LoadNStore(unsigned int _inst, EncodingType _type) : Instruction(inst, _type)
	{
		typeInst = LNS;
		if (type == I) type = (LNSType) Util::getBits(12, 14, inst);
		else type = (LNSType) (Util::getBits(12, 14, inst) + 6);
	}
};

class IntCom : Instruction
{
private:
	enum ICType
	{
		ADDI, SLLI, SLTI, SLTIU, XORI, SRLI, ORI, ANDI, SRAI,
		ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND, SUB, SRA,
		LUI, AUIPC
	};
	ICType type;

public:
	IntCom(unsigned int inst, EncodingType _type) : Instruction(inst, _type)
	{
		typeInst = IC;
		auto op = Util::getBits(0, 6, inst);
		if (op == 0b0010011)
		{
			type = (ICType) Util::getBits(12, 14, inst);
			if (type == SRLI && Util::getBits(30, 30, inst)) type = SRAI;
		}
		else if (op == 0b0110011)
		{
			type = (ICType) (Util::getBits(12, 14, inst) + 9);
			if (type == ADD && Util::getBits(30, 30, inst)) type = SUB;
			if (type == SRL && Util::getBits(30, 30, inst)) type = SRA;
		}
		else
		{
			if (op == 0b0110111) type = LUI;
			else type = AUIPC;
		}
	}
};


class InstructionParser
{
public:
	static Instruction *parseInst(unsigned int inst)
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
				std::cerr << "CANT PARSE: " << inst << std::endl;
				break;
		}
	}
};

#endif //RISCV_SIMULATOR_INSTRUCTION_HPP
