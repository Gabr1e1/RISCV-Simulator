//
// Created by Gabriel on 7/3/2019.
//

#include "instruction.h"
#include "executor.h"

Instruction::Instruction(unsigned int _inst, EncodingType _typeEnc) : inst(_inst), typeEnc(_typeEnc)
{

}

void Instruction::IF(Executor *exec)
{
	npc = exec->pc + 4;
}

void Instruction::ID(Executor *exec)
{
//	printf("INST: %x\n", inst);
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
				  (Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0);
			break;
		case S:
			imm = Util::getBits(7, 7, inst) | (Util::getBits(8, 11, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
				  (Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0);
			break;
		case B:
			imm = (Util::getBits(8, 11, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
				  (Util::getBits(7, 7, inst) << 11) | (Util::getBits(31, 31, inst) ? Util::bitmask(12, 31) : 0);
			break;
		case U:
			imm = (Util::getBits(12, 19, inst) << 12) | (Util::getBits(20, 31, inst) << 20);
			break;
		case J:
			imm = (Util::getBits(21, 24, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
				  (Util::getBits(20, 20, inst) << 11) | (Util::getBits(12, 19, inst) << 12) |
				  (Util::getBits(31, 31, inst) ? Util::bitmask(20, 31) : 0);
			break;
		default:
			imm = 0;
			break;
	}
//	std::cerr << "RS1: " << rs1 << " " << rs1v << " " << "rs2: " << rs2 << " " << rs2v << " " << "rd: " << rd << " "
//			  << "imm: " << imm << std::endl;
}


CtrlTrans::CtrlTrans(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
{
	typeInst = CT;
	auto op = Util::getBits(0, 6, inst);
	if (op == 0b1100011) type = (CTType) Util::getBits(12, 14, inst);
	else if (op == 0b1101111) type = JAL;
	else type = JALR;
}

void CtrlTrans::EX(Executor *exec)
{
	switch (type)
	{
		case JAL:
			ALUOutput = exec->pc + 4;
			exec->pc += imm;
			break;
		case JALR:
			ALUOutput = exec->pc + 4;
			exec->pc = ((unsigned) (rs1v + imm) >> 1) << 1;
			break;
		case BEQ:
			cond = (rs1v == rs2v);
			break;
		case BNE:
			cond = (rs1v != rs2v);
			break;
		case BLT:
			cond = (rs1v < rs2v);
			break;
		case BLTU:
			cond = ((unsigned) rs1v < (unsigned) rs2v);
			break;
		case BGE:
			cond = (rs1v >= rs2v);
			break;
		case BGEU:
			cond = ((unsigned) rs1v >= (unsigned) rs2v);
			break;
		default:
			break;
	}
	if (type == BEQ || type == BNE || type == BLT || type == BLTU || type == BGE || type == BGEU)
		ALUOutput = exec->pc + imm;
}

void CtrlTrans::MEM(Executor *exec)
{
	if (type == BEQ || type == BNE || type == BLT || type == BLTU || type == BGE || type == BGEU)
	{
		if (cond) exec->pc = ALUOutput;
		else exec->pc = npc;
	}
}

void CtrlTrans::WB(Executor *exec)
{
	if (type == JAL || type == JALR) exec->reg[rd] = ALUOutput;
}


LoadNStore::LoadNStore(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
{
	typeInst = LNS;
	if (typeEnc == I) type = (LNSType) Util::getBits(12, 14, inst);
	else type = (LNSType) (Util::getBits(12, 14, inst) + 6);
}

void LoadNStore::EX(Executor *exec)
{
	ALUOutput = rs1v + imm;
}

void LoadNStore::MEM(Executor *exec)
{
	exec->pc = npc;
	switch (type)
	{
		case LB:
			lmd = *reinterpret_cast<int8_t *>(exec->mem + ALUOutput);
			break;
		case LH:
			lmd = *reinterpret_cast<int16_t *>(exec->mem + ALUOutput);
			break;
		case LW:
			lmd = *reinterpret_cast<int32_t *>(exec->mem + ALUOutput);
			break;
		case LBU:
			lmd = *reinterpret_cast<uint8_t *>(exec->mem + ALUOutput);
			break;
		case LHU:
			lmd = *reinterpret_cast<uint16_t *>(exec->mem + ALUOutput);
			break;
		case SB:
			memcpy(exec->mem + ALUOutput, reinterpret_cast<char *>(&rs2v), 1);
			break;
		case SH:
			memcpy(exec->mem + ALUOutput, reinterpret_cast<char *>(&rs2v), 2);
			break;
		case SW:
			memcpy(exec->mem + ALUOutput, reinterpret_cast<char *>(&rs2v), 4);
			break;
		default:
			break;
	}
}

void LoadNStore::WB(Executor *exec)
{
	if (type == LB || type == LH || type == LW || type == LBU || type == LHU) exec->reg[rd] = lmd;
}

IntCom::IntCom(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
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

void IntCom::EX(Executor *exec)
{
	ALUOutput = 0;
	switch (type)
	{
		case ADDI:
			ALUOutput = imm + rs1v;
			break;
		case SLLI:
			ALUOutput = (int) ((unsigned) (rs1v << Util::getBits(0, 4, imm)));
			break;
		case SLTI:
			ALUOutput = (rs1v < imm);
			break;
		case SLTIU:
			ALUOutput = (rs1v < (unsigned) imm);
			break;
		case XORI:
			ALUOutput = rs1v ^ imm;
			break;
		case SRLI:
			ALUOutput = (int) ((unsigned) (rs1v >> Util::getBits(0, 4, imm)));
			break;
		case ORI:
			ALUOutput = rs1v | imm;
			break;
		case ANDI:
			ALUOutput = rs1v & imm;
			break;
		case SRAI:
			ALUOutput = rs1v >> Util::getBits(0, 4, imm);
			break;
		case ADD:
			ALUOutput = rs1v + rs2v;
			break;
		case SLL:
			ALUOutput = (int) ((unsigned) rs1v << Util::getBits(0, 4, rs2v));
			break;
		case SLT:
			ALUOutput = (rs1v < rs2v);
			break;
		case SLTU:
			ALUOutput = ((unsigned) rs1v < (unsigned) (rs2v));
			break;
		case XOR:
			ALUOutput = rs1v ^ rs2v;
			break;
		case SRL:
			ALUOutput = (int) ((unsigned) rs1v >> Util::getBits(0, 4, rs2v));
			break;
		case OR:
			ALUOutput = rs1v | rs2v;
			break;
		case AND:
			ALUOutput = rs1v & rs2v;
			break;
		case SUB:
			ALUOutput = rs1v - rs2v;
			break;
		case SRA:
			ALUOutput = rs1v >> Util::getBits(0, 4, rs2v);
			break;
		case LUI:
			ALUOutput = imm;
			break;
		case AUIPC:
			ALUOutput = imm + exec->pc;
			break;
		default:
			break;
	}
}

void IntCom::MEM(Executor *exec)
{
	exec->pc = npc;
}

void IntCom::WB(Executor *exec)
{
	exec->reg[rd] = ALUOutput;
}
