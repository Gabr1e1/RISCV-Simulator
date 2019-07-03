//
// Created by Gabriel on 7/3/2019.
//

#ifndef RISCV_SIMULATOR_INSTRUCTION_H
#define RISCV_SIMULATOR_INSTRUCTION_H

#include <iostream>
#include "utility.hpp"

enum EncodingType { R, I, S, B, U, J };
enum InstructionType { CT, LNS, IC };

class Executor;

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

protected:
	int ALUOutput;
	bool cond;
	int lmd;
	int npc;

public:
	Instruction(unsigned int _inst, EncodingType _typeEnc);

public:
	void IF(Executor *exec);
	void ID(Executor *exec);

	virtual void EX(Executor *exec) = 0;

	virtual void MEM(Executor *exec) = 0;

	virtual void WB(Executor *exec) = 0;
};

class CtrlTrans : public Instruction
{
private:
	enum CTType
	{
		BEQ, BNE, XXX, XXXX, BLT, BGE, BLTU, BGEU,
		JAL, JALR
	};
	CTType type;

public:
	CtrlTrans(unsigned int _inst, EncodingType _type);

	void EX(Executor *exec);

	void MEM(Executor *exec);

	void WB(Executor *exec);
};

class LoadNStore : public Instruction
{
private:
	enum LNSType
	{
		LB, LH, LW, XXX, LBU, LHU, SB, SH, SW
	};
	LNSType type;

public:
	LoadNStore(unsigned int _inst, EncodingType _type);

public:
	void EX(Executor *exec);

	void MEM(Executor *exec);

	void WB(Executor *exec);
};

class IntCom : public Instruction
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
	IntCom(unsigned int inst, EncodingType _type);

public:
	void EX(Executor *exec);

	void MEM(Executor *exec);

	void WB(Executor *exec);
};

#endif //RISCV_SIMULATOR_INSTRUCTION_H