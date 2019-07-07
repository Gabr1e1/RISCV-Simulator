//
// Created by Gabriel on 7/3/2019.
//

#include "instruction.h"
#include "executor.h"

Instruction::Instruction(unsigned int _inst, EncodingType _typeEnc) : inst(_inst), typeEnc(_typeEnc)
{
//	printf("New Inst: %x\n", inst);
}

void Instruction::flush(Executor *exec)
{
	exec->pipelineRegister[2][IR2] = exec->pipelineRegister[2][IR2] & 0xffffff80;
	exec->pipelineRegister[1][IR1] = 0;
}


void Instruction::modifyBHT(Executor *exec, int pc, bool taken)
{
	auto entry = Util::getBits(1, 12, (unsigned) pc);
	int res = Util::getBits(entry % 16 * 2, entry % 16 * 2 + 1, (unsigned) exec->bht[entry / 16]);
	if (taken) res += (res < 0b11);
	else res -= (res > 0b00);
	Util::writeBits(entry % 16 * 2, entry % 16 * 2 + 1, exec->bht[entry / 16], res);
}

bool Instruction::predictBranch(Executor *exec, int pc)
{
	auto entry = Util::getBits(1, 12, (unsigned) pc);
	return Util::getBits(entry % 16 * 2, entry % 16 * 2 + 1, (unsigned) exec->bht[entry / 16]) >= 0b10;
}

int Instruction::IF(Executor *exec)
{
	exec->pipelineRegister[0][IR0] = inst;

	//check branch taken or not taken, possibly stalling the pipeline
	auto op = (unsigned) exec->pipelineRegister[2][IR2] & 0b1111111;
	if (op == 0b1100011)
	{
		if (exec->pipelineRegister[2][cond2] != predictBranch(exec, exec->pipelineRegister[2][NPC2] - 4))
		{
			exec->pipelineRegister[0][NPC0] = exec->pc = exec->pipelineRegister[2][ALUOutput2];
			flush(exec);
			exec->miss++;
			modifyBHT(exec, exec->pipelineRegister[2][NPC2] - 4, (bool)exec->pipelineRegister[2][cond2]);
			return 2;
		}
		modifyBHT(exec, exec->pipelineRegister[2][NPC2] - 4, (bool)exec->pipelineRegister[2][cond2]);
	}
	if ((op == 0b1101111 || op == 0b1100111) && exec->pipelineRegister[2][cond2])
	{
		flush(exec);
		return 2;
	}

	//decide to jump or not
	op = Util::getBits(0, 6, (unsigned) exec->pipelineRegister[1][IR1]);
	if (op == 0b1100011)
	{
		exec->total++;
		if (predictBranch(exec, exec->pipelineRegister[1][NPC1] - 4))
		{
			auto target = exec->pipelineRegister[1][NPC1] + exec->pipelineRegister[1][Imm1] -
						  4; //should have been calculated in ID phase
			if (target != exec->pc)
			{
				exec->pipelineRegister[0][NPC0] = exec->pc = target;
				return 1;
			}
		}
	}

	exec->pipelineRegister[0][NPC0] = exec->pc = exec->pc + 4;
	return 0;
}

void Instruction::ID(Executor *exec)
{
	exec->pipelineRegister[1][IR1] = exec->pipelineRegister[0][IR0];
	exec->pipelineRegister[1][NPC1] = exec->pipelineRegister[0][NPC0];

	int rs1 = Util::getBits(15, 19, (unsigned) exec->pipelineRegister[1][IR1]);
	exec->pipelineRegister[1][A1] = exec->reg[rs1];

	int rs2 = Util::getBits(20, 24, (unsigned) exec->pipelineRegister[1][IR1]);
	exec->pipelineRegister[1][B1] = exec->reg[rs2];

//	printf("Using: %d %d %d\n", rs1, rs2, rd);

	auto inst = (unsigned) exec->pipelineRegister[1][IR1];
	switch (typeEnc)
	{
		case I:
			exec->pipelineRegister[1][Imm1] =
					Util::getBits(20, 30, inst) | (Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0);
			break;
		case S:
			exec->pipelineRegister[1][Imm1] = Util::getBits(7, 11, inst) | (Util::getBits(25, 30, inst) << 5) |
											  (Util::getBits(31, 31, inst) ? Util::bitmask(11, 31) : 0);
			break;
		case B:
			exec->pipelineRegister[1][Imm1] = (Util::getBits(8, 11, inst) << 1) | (Util::getBits(25, 30, inst) << 5) |
											  (Util::getBits(7, 7, inst) << 11) |
											  (Util::getBits(31, 31, inst) ? Util::bitmask(12, 31) : 0);
			break;
		case U:
			exec->pipelineRegister[1][Imm1] = (Util::getBits(12, 31, inst) << 12);
			break;
		case J:
			exec->pipelineRegister[1][Imm1] = (Util::getBits(21, 30, inst) << 1) | (Util::getBits(20, 20, inst) << 11) |
											  (Util::getBits(12, 19, inst) << 12) |
											  (Util::getBits(31, 31, inst) ? Util::bitmask(20, 31) : 0);
			break;
		default:
			exec->pipelineRegister[1][Imm1] = 0;
			break;
	}
//	std::cerr << "RS1: " << rs1 << " " << exec->pipelineRegister[1][A1] << " " << "rs2: " << rs2 << " " << exec->pipelineRegister[1][B1] << " " << "rd: " << rd << " "
//			  << "imm: " << imm << std::endl;
}

void Instruction::getForwardResult(Executor *exec)
{
	int rs1 = Util::getBits(15, 19, (unsigned) exec->pipelineRegister[1][IR1]);
	int rs2 = Util::getBits(20, 24, (unsigned) exec->pipelineRegister[1][IR1]);

//	could already be written to register
	if (rs1) exec->pipelineRegister[1][A1] = exec->reg[rs1];
	if (rs2) exec->pipelineRegister[1][B1] = exec->reg[rs2];

//	MEM stage
	int rd = Util::getBits(7, 11, (unsigned) exec->pipelineRegister[3][IR3]);
	int op = exec->pipelineRegister[3][IR3] & 0b1111111;

	if ((rs1 == rd && rd != 0) && (op == 0b0110111 || op == 0b0110011 || op == 0b0010011 || op == 0b1101111 ||
								   op == 0b1100111)) //LUI, JAL, JALR, IntCom could change the value in MEM stage
		exec->pipelineRegister[1][A1] = exec->pipelineRegister[3][ALUOutput3];
	if ((rs1 == rd && rd != 0) && op == 0b0000011) //Load could change the value in MEM stage
		exec->pipelineRegister[1][A1] = exec->pipelineRegister[3][LMD3];

	if ((rs2 == rd && rd != 0) && (op == 0b0110111 || op == 0b0110011 || op == 0b0010011 || op == 0b1101111 ||
								   op == 0b1100111)) //JAL, JALR, IntCom could change the value in MEM stage
		exec->pipelineRegister[1][B1] = exec->pipelineRegister[3][ALUOutput3];
	if ((rs2 == rd && rd != 0) && op == 0b0000011) //Load could change the value in MEM stage
		exec->pipelineRegister[1][B1] = exec->pipelineRegister[3][LMD3];

//	ALU stage
	rd = Util::getBits(7, 11, (unsigned) exec->pipelineRegister[2][IR2]);
	op = exec->pipelineRegister[2][IR2] & 0b1111111;

	if ((rs1 == rd && rd != 0) && (op == 0b0110111 || op == 0b0110011 || op == 0b0010011 || op == 0b1101111 ||
								   op == 0b1100111)) //JAL, JALR, IntCom could change the value in ALU stage
		exec->pipelineRegister[1][A1] = exec->pipelineRegister[2][ALUOutput2];
	if ((rs2 == rd && rd != 0) && (op == 0b0110111 || op == 0b0110011 || op == 0b0010011 || op == 0b1101111 ||
								   op == 0b1100111)) //JAL, JALR, IntCom could change the value in ALU stage
		exec->pipelineRegister[1][B1] = exec->pipelineRegister[2][ALUOutput2];
}

CtrlTrans::CtrlTrans(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
{
	auto op = inst & 0b1111111;
	if (op == 0b1100011) type = (CTType) Util::getBits(12, 14, inst);
	else if (op == 0b1101111) type = JAL;
	else type = JALR;
}

void CtrlTrans::EX(Executor *exec)
{
	getForwardResult(exec);
	exec->pipelineRegister[2][IR2] = exec->pipelineRegister[1][IR1];
	exec->pipelineRegister[2][NPC2] = exec->pipelineRegister[1][NPC1];
	switch (type)
	{
		case JAL:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][NPC1];
			exec->pc = exec->pipelineRegister[1][NPC1] + exec->pipelineRegister[1][Imm1] - 4;
			exec->pipelineRegister[2][cond2] = true;
			break;
		case JALR:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][NPC1];
			exec->pc = (unsigned) ((exec->pipelineRegister[1][A1] + exec->pipelineRegister[1][Imm1])) & 0xfffffffe;
			exec->pipelineRegister[2][cond2] = true;
			break;
		case BEQ:
			exec->pipelineRegister[2][cond2] = (exec->pipelineRegister[1][A1] == exec->pipelineRegister[1][B1]);
			break;
		case BNE:
			exec->pipelineRegister[2][cond2] = (exec->pipelineRegister[1][A1] != exec->pipelineRegister[1][B1]);
			break;
		case BLT:
			exec->pipelineRegister[2][cond2] = (exec->pipelineRegister[1][A1] < exec->pipelineRegister[1][B1]);
			break;
		case BLTU:
			exec->pipelineRegister[2][cond2] = ((unsigned) exec->pipelineRegister[1][A1] <
												(unsigned) exec->pipelineRegister[1][B1]);
			break;
		case BGE:
			exec->pipelineRegister[2][cond2] = (exec->pipelineRegister[1][A1] >= exec->pipelineRegister[1][B1]);
			break;
		case BGEU:
			exec->pipelineRegister[2][cond2] = ((unsigned) exec->pipelineRegister[1][A1] >=
												(unsigned) exec->pipelineRegister[1][B1]);
			break;
		default:
			break;
	}
	if (type == BEQ || type == BNE || type == BLT || type == BLTU || type == BGE || type == BGEU)
		exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[2][cond2] ? (exec->pipelineRegister[1][NPC1] +
																					exec->pipelineRegister[1][Imm1] - 4)
																				 : exec->pipelineRegister[1][NPC1];
}

void CtrlTrans::MEM(Executor *exec)
{
	exec->pipelineRegister[3][IR3] = exec->pipelineRegister[2][IR2];
	exec->pipelineRegister[3][ALUOutput3] = exec->pipelineRegister[2][ALUOutput2];
}

void CtrlTrans::WB(Executor *exec)
{
	int rd = Util::getBits(7, 11, (unsigned int) exec->pipelineRegister[3][IR3]);
	if (type == JAL || type == JALR)
	{
		exec->reg[rd] = exec->pipelineRegister[3][ALUOutput3];
//		printf("JAL Modified %d %d\n", rd, exec->reg[rd]);
	}
}


LoadNStore::LoadNStore(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
{
	if (typeEnc == I) type = (LNSType) Util::getBits(12, 14, inst);
	else type = (LNSType) (Util::getBits(12, 14, inst) + 6);
}

void LoadNStore::EX(Executor *exec)
{
	getForwardResult(exec);
	exec->pipelineRegister[2][IR2] = exec->pipelineRegister[1][IR1];
	exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][A1] + exec->pipelineRegister[1][Imm1];
	exec->pipelineRegister[2][B2] = exec->pipelineRegister[1][B1];
}

void LoadNStore::MEM(Executor *exec)
{
	exec->pipelineRegister[3][IR3] = exec->pipelineRegister[2][IR2];
	switch (type)
	{
		case LB:
			exec->pipelineRegister[3][LMD3] = *reinterpret_cast<int8_t *>(exec->mem +
																		  exec->pipelineRegister[2][ALUOutput2]);
			break;
		case LH:
			exec->pipelineRegister[3][LMD3] = *reinterpret_cast<int16_t *>(exec->mem +
																		   exec->pipelineRegister[2][ALUOutput2]);
			break;
		case LW:
			exec->pipelineRegister[3][LMD3] = *reinterpret_cast<int32_t *>(exec->mem +
																		   exec->pipelineRegister[2][ALUOutput2]);
			break;
		case LBU:
			exec->pipelineRegister[3][LMD3] = *reinterpret_cast<uint8_t *>(exec->mem +
																		   exec->pipelineRegister[2][ALUOutput2]);
			break;
		case LHU:
			exec->pipelineRegister[3][LMD3] = *reinterpret_cast<uint16_t *>(exec->mem +
																			exec->pipelineRegister[2][ALUOutput2]);
			break;
		case SB:
			memcpy(exec->mem + exec->pipelineRegister[2][ALUOutput2],
				   reinterpret_cast<char *>(&exec->pipelineRegister[2][B2]), 1);
			break;
		case SH:
			memcpy(exec->mem + exec->pipelineRegister[2][ALUOutput2],
				   reinterpret_cast<char *>(&exec->pipelineRegister[2][B2]), 2);
			break;
		case SW:
			memcpy(exec->mem + exec->pipelineRegister[2][ALUOutput2],
				   reinterpret_cast<char *>(&exec->pipelineRegister[2][B2]), 4);
			break;
		default:
			break;
	}
}

void LoadNStore::WB(Executor *exec)
{
	int rd = Util::getBits(7, 11, (unsigned int) exec->pipelineRegister[3][IR3]);
	if (type == LB || type == LH || type == LW || type == LBU || type == LHU)
	{
		exec->reg[rd] = exec->pipelineRegister[3][LMD3];
	}
}

IntCom::IntCom(unsigned int _inst, EncodingType _type) : Instruction(_inst, _type)
{
	auto op = inst & 0b1111111;
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
	getForwardResult(exec);
	exec->pipelineRegister[2][IR2] = exec->pipelineRegister[1][IR1];
	switch (type)
	{
		case ADDI:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][Imm1] + exec->pipelineRegister[1][A1];
			break;
		case SLLI:
			exec->pipelineRegister[2][ALUOutput2] =
					exec->pipelineRegister[1][A1] << Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][Imm1]);
			break;
		case SLTI:
			exec->pipelineRegister[2][ALUOutput2] = (exec->pipelineRegister[1][A1] < exec->pipelineRegister[1][Imm1]);
			break;
		case SLTIU:
			exec->pipelineRegister[2][ALUOutput2] = ((unsigned) exec->pipelineRegister[1][A1] <
													 (unsigned) exec->pipelineRegister[1][Imm1]);
			break;
		case XORI:
			exec->pipelineRegister[2][ALUOutput2] =
					(unsigned) exec->pipelineRegister[1][A1] ^ (unsigned) exec->pipelineRegister[1][Imm1];
			break;
		case SRLI:
			exec->pipelineRegister[2][ALUOutput2] = (int) (((unsigned) exec->pipelineRegister[1][A1])
					>> Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][Imm1]));
			break;
		case ORI:
			exec->pipelineRegister[2][ALUOutput2] =
					(unsigned) exec->pipelineRegister[1][A1] | (unsigned) exec->pipelineRegister[1][Imm1];
			break;
		case ANDI:
			exec->pipelineRegister[2][ALUOutput2] =
					(unsigned) exec->pipelineRegister[1][A1] & (unsigned) exec->pipelineRegister[1][Imm1];
			break;
		case SRAI:
			exec->pipelineRegister[2][ALUOutput2] =
					exec->pipelineRegister[1][A1] >> Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][Imm1]);
			break;
		case ADD:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][A1] + exec->pipelineRegister[1][B1];
			break;
		case SLL:
			exec->pipelineRegister[2][ALUOutput2] = (int) (((unsigned) exec->pipelineRegister[1][A1])
					<< Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][B1]));
			break;
		case SLT:
			exec->pipelineRegister[2][ALUOutput2] = (exec->pipelineRegister[1][A1] < exec->pipelineRegister[1][B1]);
			break;
		case SLTU:
			exec->pipelineRegister[2][ALUOutput2] = ((unsigned) exec->pipelineRegister[1][A1] <
													 (unsigned) (exec->pipelineRegister[1][B1]));
			break;
		case XOR:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][A1] ^ exec->pipelineRegister[1][B1];
			break;
		case SRL:
			exec->pipelineRegister[2][ALUOutput2] = (int) (((unsigned) exec->pipelineRegister[1][A1])
					>> Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][B1]));
			break;
		case OR:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][A1] | exec->pipelineRegister[1][B1];
			break;
		case AND:
			exec->pipelineRegister[2][ALUOutput2] =
					(unsigned) exec->pipelineRegister[1][A1] & (unsigned) exec->pipelineRegister[1][B1];
			break;
		case SUB:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][A1] - exec->pipelineRegister[1][B1];
			break;
		case SRA:
			exec->pipelineRegister[2][ALUOutput2] =
					exec->pipelineRegister[1][A1] >> Util::getBits(0, 4, (unsigned) exec->pipelineRegister[1][B1]);
			break;
		case LUI:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][Imm1];
			break;
		case AUIPC:
			exec->pipelineRegister[2][ALUOutput2] = exec->pipelineRegister[1][Imm1] + exec->pc;
			break;
		default:
			break;
	}
}

void IntCom::MEM(Executor *exec)
{
	exec->pipelineRegister[3][IR3] = exec->pipelineRegister[2][IR2];
	exec->pipelineRegister[3][ALUOutput3] = exec->pipelineRegister[2][ALUOutput2];
}

void IntCom::WB(Executor *exec)
{
	int rd = Util::getBits(7, 11, (unsigned int) exec->pipelineRegister[3][IR3]);
//	printf("Modified %d to %d\n", rd, exec->pipelineRegister[3][ALUOutput3]);
	exec->reg[rd] = exec->pipelineRegister[3][ALUOutput3];
}
