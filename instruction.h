//
// Created by Gabriel on 7/3/2019.
//

#ifndef RISCV_SIMULATOR_INSTRUCTION_H
#define RISCV_SIMULATOR_INSTRUCTION_H

#include <iostream>
#include "utility.hpp"

enum EncodingType
{
	R, I, S, B, U, J
};

enum IFIDRegister
{
	IR0, NPC0
};
enum IDEXRegister
{
	IR1, NPC1, A1, B1, Imm1, JmpTarget1
};
enum EXMEMRegister
{
	IR2, NPC2, ALUOutput2, cond2, B2
};
enum MEMWBRegister
{
	IR3, ALUOutput3, LMD3
};

class Executor;

class Instruction
{
public:
	EncodingType typeEnc;
	unsigned int inst;

public:
	Instruction(unsigned int _inst, EncodingType _typeEnc);

public:
	void flush(Executor *exec);

	void getForwardResult(Executor *exec);

	bool predictBranch(Executor *exec, int pc);

	int IF(Executor *exec);

	void ID(Executor *exec);

	virtual void EX(Executor *exec) = 0;

	virtual int MEM(Executor *exec) = 0;

	virtual void WB(Executor *exec) = 0;

private:
	void modifyBHT(Executor *exec, int pc, bool curResult);
};

class CtrlTrans : public Instruction
{
private:
	enum CTType
	{
		BEQ, BNE, XXX, XXXX, BLT, BGE, BLTU, BGEU, JAL, JALR
	};
	CTType type;

public:
	CtrlTrans(unsigned int _inst, EncodingType _type);

	void EX(Executor *exec);

	int MEM(Executor *exec);

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

	int MEM(Executor *exec);

	void WB(Executor *exec);
};

class IntCom : public Instruction
{
private:
	enum ICType
	{
		ADDI,
		SLLI,
		SLTI,
		SLTIU,
		XORI,
		SRLI,
		ORI,
		ANDI,
		SRAI,
		ADD,
		SLL,
		SLT,
		SLTU,
		XOR,
		SRL,
		OR,
		AND,
		SUB,
		SRA,
		LUI,
		AUIPC
	};
	ICType type;

public:
	IntCom(unsigned int inst, EncodingType _type);

public:
	void EX(Executor *exec);

	int MEM(Executor *exec);

	void WB(Executor *exec);
};

#endif //RISCV_SIMULATOR_INSTRUCTION_H
