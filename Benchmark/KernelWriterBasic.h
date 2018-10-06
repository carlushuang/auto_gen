#pragma once

#include "BasicClass.h"

namespace krnelWriter
{
#define	PARAM_START_COL		(44)
#define	FLAG_START_COL		(85)
#define	COMMON_START_COL	(109)

#define	c2s(c)				std::string(c)
#define	s2c(s)				s.c_str()
#define	d2s(d)				std::to_string(d)
#define d2c(d)				(char*)(s2c(d2s(d)))
#define	d2hs(d)				std::string("0x0+"+std::to_string(d))
#define	c2hs(c)				std::string("0x0+"+c2s(c))

	typedef enum IsaArchEnum
	{
		Gfx800 = 800,
		Gfx900 = 900
	}E_IsaArch;
	
	typedef enum VarTypeEnum
	{
		VAR_SGPR = 1,
		VAR_VGPR = 2,
		VAR_IMM = 3,
		VAR_LABER = 4,
		VAR_STRING = 5,
		VAR_OFF = 0
	}E_VarType;

	typedef struct GprType
	{
		std::string name;
		int perGprIdx;
		int gprIdx;
		int len;
		int align;
		bool forceLen;
	}t_gpr;

	typedef struct ImmType
	{
		std::string name;
		int value;
	}t_imm;

	typedef struct LaberType
	{
		std::string laber;
	}t_lab;

	typedef struct VarType
	{
		std::string name;
		t_gpr sgpr;
		t_gpr vgpr;
		t_imm imm;
		t_lab laber;
		E_VarType type;

		VarType * operator+(int i)
		{
			struct VarType * np = new VarType();
			np->name = this->name + "+" + d2s(i);
			np->type = this->type;
			np->sgpr = this->sgpr;
			np->vgpr = this->vgpr;
			np->imm = this->imm;
			np->laber = this->laber;

			if (this->type == E_VarType::VAR_SGPR)
				np->sgpr.gprIdx = this->sgpr.gprIdx + i;
			else if (this->type == E_VarType::VAR_VGPR)
				np->vgpr.gprIdx = this->vgpr.gprIdx + i;

			return np;
		}
		VarType * operator^(int i)
		{
			if (this->type == E_VarType::VAR_SGPR)
			{
				this->sgpr.forceLen = true;
			}
			else if (this->type == E_VarType::VAR_VGPR)
			{
				this->vgpr.forceLen = true;
			}

			return this;
		}
	}Var;

	typedef enum OpTypeEnum
	{
		OP_ADD = 1,
		OP_SUB = 2,
		OP_INC = 3,
		OP_DEC = 4,
		OP_OR = 5,
		OP_XOR = 6,
		OP_AND = 7,
		OP_SMAX = 8,
		OP_SMIN = 9,
		OP_UMAX = 10,
		OP_UMIN = 11,
		OP_SWAP = 12,
		OP_CMPSWAP = 13,
	}E_OpType;
	
	class KernelWriterBasic
	{
	public:
		KernelWriterBasic(E_IsaArch isaArch = E_IsaArch::Gfx900)
		{
			sgprCount = 0;
			vgprCount = 0;
			ldsByteCount = 0;
			memset(SgprState, 0, sizeof(int)*MAX_SGPR_COUNT);
			memset(VgprState, 0, sizeof(int)*MAX_VGPR_COUNT);
			OperatorMap = new std::map<std::string, Var*>();
			IsaArch = isaArch;
		}
		std::string * GetKernelString()
		{
			return &KernelString;
		}

	protected:
		std::string KernelString;

		/************************************************************************/
		/* 寄存器与变量操作                                                       */
		/************************************************************************/
		int SgprState[MAX_SGPR_COUNT];
		int VgprState[MAX_VGPR_COUNT];
		int sgprCount = 0, sgprCountMax = 0;
		int vgprCount = 0, vgprCountMax = 0;
		int ldsByteCount = 0;
		std::map<std::string, Var*> *OperatorMap;

		Var * newSgpr(std::string name, int len = 1, int align = 1)
		{
			Var *opt;

			if ((name == "off") || (name == "OFF"))
			{
				opt = new Var;
				opt->name = name;
				opt->type = E_VarType::VAR_OFF;
			}
			else
			{
				int idleIdx;
				for (idleIdx = 0; idleIdx < MAX_SGPR_COUNT; idleIdx++)
				{
					if (idleIdx % align != 0)
						continue;

					if (SgprState[idleIdx] != 0)
						continue;

					int idleChk;
					for (idleChk = 0; idleChk < len; idleChk++)
					{
						if (SgprState[idleChk + idleIdx] != 0)
							break;
					}
					if (idleChk != len)
						continue;
					break;
				}
				if (idleIdx == MAX_SGPR_COUNT)
					return nullptr;

				for (int i = 0; i < len; i++)
				{
					SgprState[idleIdx + i] = 1;
				}

				opt = new Var;
				opt->name = name;
				opt->type = E_VarType::VAR_SGPR;
				opt->sgpr.name = name;
				opt->sgpr.perGprIdx = sgprCount;
				opt->sgpr.gprIdx = idleIdx;
				opt->sgpr.len = len;
				opt->sgpr.align = align;
				opt->sgpr.forceLen = false;

				if (idleIdx + len > sgprCountMax)
					sgprCountMax = idleIdx + len;
			}

			OperatorMap->insert(std::pair<std::string, Var*>(name, opt));
			return opt;
		}
		Var * newVgpr(std::string name, int len = 1, int align = 1)
		{
			Var *opt;

			if ((name == "off") || (name == "OFF"))
			{
				opt = new Var;
				opt->name = name;
				opt->type = E_VarType::VAR_OFF;
			}
			else
			{
				int idleIdx;
				for (idleIdx = 0; idleIdx < MAX_VGPR_COUNT; idleIdx++)
				{
					if (idleIdx % align != 0)
						continue;

					if (VgprState[idleIdx] != 0)
						continue;

					int idleChk;
					for (idleChk = 0; idleChk < len; idleChk++)
					{
						if (VgprState[idleChk + idleIdx] != 0)
							break;
					}
					if (idleChk != len)
						continue;
					break;
				}
				if (idleIdx == MAX_VGPR_COUNT)
					return nullptr;

				for (int i = 0; i < len; i++)
				{
					VgprState[idleIdx + i] = 1;
				}

				opt = new Var;
				opt->name = name;
				opt->type = E_VarType::VAR_VGPR;
				opt->vgpr.name = name;
				opt->vgpr.perGprIdx = sgprCount;
				opt->vgpr.gprIdx = idleIdx;
				opt->vgpr.len = len;
				opt->vgpr.align = align;
				opt->vgpr.forceLen = false;

				if (idleIdx + len > vgprCountMax)
					vgprCountMax = idleIdx + len;
			}

			OperatorMap->insert(std::pair<std::string, Var*>(name, opt));
			return opt;
		}
		Var * newImm(std::string name, int val = 0)
		{
			Var *opt = new Var;

			opt->name = name;
			opt->type = E_VarType::VAR_IMM;
			opt->imm.name = name;
			opt->imm.value = val;

			OperatorMap->insert(std::pair<std::string, Var*>(name, opt));
			return opt;
		}
		Var * newLaber(std::string laber)
		{
			Var *opt;
			opt = new Var;
			opt->name = laber;
			opt->type = E_VarType::VAR_LABER;
			opt->laber.laber = laber;
			OperatorMap->insert(std::pair<std::string, Var*>(laber, opt));
			return opt;
		}
		void delVar(Var * opter)
		{
			if (opter->type == E_VarType::VAR_SGPR)
			{
				int gprIdx = opter->sgpr.gprIdx;
				for (int i = 0; i < opter->sgpr.len; i++)
				{
					SgprState[gprIdx + i] = 0;
				}
			}
			else if (opter->type == E_VarType::VAR_VGPR)
			{
				int gprIdx = opter->vgpr.gprIdx;
				for (int i = 0; i < opter->vgpr.len; i++)
				{
					VgprState[gprIdx + i] = 0;
				}
			}
			OperatorMap->erase(opter->name);
			delete(opter);
		}
		void clrVar()
		{
			memset(SgprState, 0, sizeof(int)*MAX_SGPR_COUNT);
			memset(VgprState, 0, sizeof(int)*MAX_VGPR_COUNT);
			OperatorMap->clear();
		}

		std::string getVar(Var * opter, int len = 1)
		{
			if (opter->type == E_VarType::VAR_SGPR)
			{
				if (opter->sgpr.forceLen == true)
				{
					opter->sgpr.forceLen = false;
					return std::string("s[" + d2s(opter->sgpr.gprIdx) + ":" + d2s(opter->sgpr.gprIdx + opter->sgpr.len - 1) + "]");
				}
				else
				{
					if (len == 1)
					{
						return std::string("s[" + d2s(opter->sgpr.gprIdx) + "]");
					}
					else
					{
						return std::string("s[" + d2s(opter->sgpr.gprIdx) + ":" + d2s(opter->sgpr.gprIdx + len - 1) + "]");
					}
				}
			}
			else if (opter->type == E_VarType::VAR_VGPR)
			{
				if (opter->vgpr.forceLen == true)
				{
					opter->vgpr.forceLen = false;
					return std::string("v[" + d2s(opter->vgpr.gprIdx) + ":" + d2s(opter->vgpr.gprIdx + opter->vgpr.len - 1) + "]");
				}
				else
				{
					if (len == 1)
					{
						return std::string("v[" + d2s(opter->vgpr.gprIdx) + "]");
					}
					else
					{
						std::string("v[" + d2s(opter->vgpr.gprIdx) + ":" + d2s(opter->vgpr.gprIdx + len - 1) + "]");
					}
				}
			}
			else if (opter->type == E_VarType::VAR_IMM)
			{
				return std::string(d2s(opter->imm.value));
			}
			else if (opter->type == E_VarType::VAR_LABER)
			{
				return opter->laber.laber;
			}
		}
		std::string getVar(std::string name, int len = 1)
		{
			if ((name == "vcc") || (name == "vcc_hi") || (name == "vcc_lo") ||
				(name == "exec") || (name == "exec_hi") || (name == "exec_lo") ||
				(name == "off")||(name == "m0"))
			{
				return name;
			}
			if ((name[0] == '0') && (name[1] == 'x'))
			{
				return name;
			}
		}
		std::string getVar(double immVal, int len = 1)
		{
			if((immVal - (int)immVal) != 0)
				return d2s((float)immVal);
			else
				return d2s((int)immVal);
		}

		E_VarType getVarType(Var * opter) { return opter->type; }
		E_VarType getVarType(std::string name) 
		{ 
			if ((name == "vcc") || (name == "vcc_hi") || (name == "vcc_lo") ||
				(name == "exec") || (name == "exec_hi") || (name == "exec_lo"))
			{
				return E_VarType::VAR_STRING;
			}
			else if (name == "off")
			{
				return E_VarType::VAR_OFF;
			}
		}
		E_VarType getVarType(double immVal) { return E_VarType::VAR_IMM;}
		/************************************************************************/
		/* 通用函数		                                                        */
		/************************************************************************/
		int tableCnt = 0;
		std::string sblk()
		{
			std::string str = "";
			for (int i = 0; i < tableCnt; i++)
			{
				str.append("    ");
			}
			return str;
		}
		void indent()
		{
			tableCnt++;
		}
		void backSpace()
		{
			tableCnt--;
		}
		void setTable(int tab)
		{
			tableCnt = tab;
		}
		
		void wrString(std::string str)
		{
			std::string tmp = sblk();
			tmp.append(str);
			tmp.append("\n");
			KernelString.append(tmp);
		}
		void wrLine(std::string line)
		{
			std::string tmp = sblk();
			tmp.append(line);
			tmp.append("\n");
			KernelString.append(tmp);
		}
		void clearString()
		{
			KernelString.clear();
		}
		
		void wrCommom1(std::string common)
		{
			KernelString.append("/************************************************************************************/\n");
			KernelString.append("/* " + common + " */\n");
			KernelString.append("/************************************************************************************/\n");
		}
		void wrCommom2(std::string common)
		{
			KernelString.append("// ==================================================================================\n");
			KernelString.append("// " + common + " \n");
			KernelString.append("// ==================================================================================\n");
		}
		void wrCommom3(std::string common)
		{
			KernelString.append("// ----------------------------------------------------------------------------------\n");
			KernelString.append("// " + common + " \n");
			KernelString.append("// ----------------------------------------------------------------------------------\n");
		}

		int log2(int value)
		{
			int log2 = 0;
			while (value > 1)
			{
				value = value / 2;
				log2++;
			}
			return log2;
		}
		int modMask(int value)
		{
			return value - 1;
		}

#pragma region ISA_REGION
		E_IsaArch IsaArch = E_IsaArch::Gfx900;
		/************************************************************************************/
		/* SMEM																				*/
		/************************************************************************************/
		template <typename T>
		E_ReturnState s_load_dword(int num, Var* s_dst, Var* s_base, T offset, bool glc = true)
		{
			int tmpIdx;
			std::string str = "";
			str.append("s_load_dword");
			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			if (num == 1)
			{
				str.append(getVar(s_dst));
			}
			else
			{
				str.append(getVar(s_dst, num));
			}
			str.append(", ");
			str.append(getVar(s_base, 2));
			str.append(", ");
			str.append(getVar(offset));

			if (glc != true)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("glc");
			}

			wrLine(str);

			if (s_dst->type != E_VarType::VAR_SGPR)
			{
				str.append("dest reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_base->type != E_VarType::VAR_SGPR)
			{
				str.append("base addr reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_base->sgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (getVarType(offset) == E_VarType::VAR_VGPR)
			{
				str.append("offset reg are vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		template <typename T>
		E_ReturnState s_store_dword(int num, Var* s_dst, Var* s_base, T offset, bool glc = true)
		{
			int tmpIdx;
			std::string str = "";
			str.append("s_store_dword");
			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			if (num == 1)
			{
				str.append(getVar(s_dst));
			}
			else
			{
				str.append(getVar(s_dst, num));
			}
			str.append(", ");
			str.append(getVar(s_base, 2));
			str.append(", ");
			str.append(getVar(offset));

			if (glc != true)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("glc");
			}

			wrLine(str);

			if (s_dst->type != E_VarType::VAR_SGPR)
			{
				str.append("dest reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_base->type != E_VarType::VAR_SGPR)
			{
				str.append("base addr reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_base->sgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (getVarType(offset) == E_VarType::VAR_VGPR)
			{
				str.append("offset reg are vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		template <typename T>
		E_ReturnState s_atomic_op(E_OpType op, Var* s_dat, Var* s_addr, T offset, bool glc = true)
		{
			int tmpIdx;
			std::string str = "";
			str.append("s_atomic_");
			switch (op)
			{
			case OP_ADD:
				str.append("add");
				break;
			case OP_INC:
				str.append("inc");
				break;
			case OP_DEC:
				str.append("dec");
				break;
			case OP_SUB:
				str.append("sub");
				break;
			case OP_AND:
				str.append("and");
				break;
			case OP_OR:
				str.append("or");
				break;
			case OP_XOR:
				str.append("xor");
				break;
			case OP_SMAX:
				str.append("smax");
				break;
			case OP_UMAX:
				str.append("umax");
				break;
			case OP_SMIN:
				str.append("smin");
				break;
			case OP_UMIN:
				str.append("umin");
				break;
			case OP_SWAP:
				str.append("swap");
				break;
			case OP_CMPSWAP:
				str.append("cmpswap");
				break;
			default:
				str.append("invalid op");
				return E_ReturnState::FAIL;
			}

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(s_dat));
			str.append(", ");
			str.append(getVar(s_addr, 2));
			str.append(", ");
			str.append(getVar(offset));

			if (glc != true)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("glc");
			}

			wrLine(str);

			if (s_dat->type != E_VarType::VAR_SGPR)
			{
				str.append("dest reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_addr->type != E_VarType::VAR_SGPR)
			{
				str.append("base addr reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_addr->sgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (getVarType(offset) == E_VarType::VAR_VGPR)
			{
				str.append("offset reg are vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		template <typename T>
		E_ReturnState s_atomic_op2(E_OpType op, Var* s_dat, Var* s_addr, T offset, bool glc = true)
		{
			int tmpIdx;
			std::string str = "";
			str.append("s_atomic_");
			switch (op)
			{
			case OP_ADD:
				str.append("add");
				break;
			case OP_INC:
				str.append("inc");
				break;
			case OP_DEC:
				str.append("dec");
				break;
			case OP_SUB:
				str.append("sub");
				break;
			case OP_AND:
				str.append("and");
				break;
			case OP_OR:
				str.append("or");
				break;
			case OP_XOR:
				str.append("xor");
				break;
			case OP_SMAX:
				str.append("smax");
				break;
			case OP_UMAX:
				str.append("umax");
				break;
			case OP_SMIN:
				str.append("smin");
				break;
			case OP_UMIN:
				str.append("umin");
				break;
			case OP_SWAP:
				str.append("swap");
				break;
			case OP_CMPSWAP:
				str.append("cmpswap");
				break;
			default:
				str.append("invalid op");
				return E_ReturnState::FAIL;
			}

			str.append("_x2");
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(s_dat, 2));
			str.append(", ");
			str.append(getVar(s_addr, 2));
			str.append(", ");
			str.append(getVar(offset));

			if (glc != true)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("glc");
			}

			wrLine(str);

			if (s_dat->type != E_VarType::VAR_SGPR)
			{
				str.append("dest reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_addr->type != E_VarType::VAR_SGPR)
			{
				str.append("base addr reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_addr->sgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (getVarType(offset) == E_VarType::VAR_VGPR)
			{
				str.append("offset reg are vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}

		/************************************************************************************/
		/* MUBUF																			*/
		/************************************************************************************/
		template <typename T>
		E_ReturnState buffer_load_dword(
			int num, Var * v_dst, 
			T v_offset_idx, 
			Var * s_desc, 
			Var * s_base_offset,
			bool idx_en, bool off_en, 
			uint i_offset)
		{
			int tmpIdx;
			std::string str = "";
			str.append("buffer_load_dword");
			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			// dest_data
			if (num == 1)
			{
				str.append(getVar(v_dst));
			}
			else
			{
				str.append(getVar(v_dst, num));
			}

			// v_index & v_offset
			str.append(", ");
			if ((idx_en == false) && (off_en == false))
			{
				str.append(getVar("off"));
			}
			else if ((idx_en == false) && (off_en == true))
			{
				str.append(getVar(v_offset_idx));
			}
			else if ((idx_en == true) && (off_en == false))
			{
				str.append(getVar(v_offset_idx));
			}
			else if ((idx_en == true) && (off_en == true))
			{
				str.append(getVar(v_offset_idx, 2));
			}

			// s_buffer_descripter
			str.append(", ");
			str.append(getVar(s_desc, 4));

			// s_base_offset
			str.append(", ");
			str.append(getVar(s_base_offset));

			if (idx_en == true)
			{
				str.append(" ");
				str.append("idxen");
			}
			if (off_en == true)
			{
				str.append(" ");
				str.append("offen");
			}

			// inst_offset
			str.append(" ");
			str.append("offset:" + d2s(i_offset));

			wrLine(str);

			//error check 
			str = "";
			if (v_dst->type != E_VarType::VAR_VGPR)
			{
				str.append("dest reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if ((idx_en == true) || (off_en == true))
			{
				if (getVarType(v_offset_idx) != E_VarType::VAR_VGPR)
				{
					str.append("offset and index reg not vgpr");
					wrLine(str);
					return E_ReturnState::FAIL;
				}
			}
			if (s_desc->type != E_VarType::VAR_SGPR)
			{
				str.append("buffer descriptor reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_desc->sgpr.len != 4)
			{
				str.append("buffer descriptor reg not 4-dword");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (s_base_offset->type != E_VarType::VAR_SGPR)
			{
				str.append("buffer obj base offset addr reg not sgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (!((i_offset >= 0) && (i_offset <= 4095)))
			{
				str.append("imm_offset is not 12-bit uint");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			
			return E_ReturnState::SUCCESS;
		}

		/************************************************************************************/
		/* FLAT																				*/
		/************************************************************************************/
		E_ReturnState flat_load_dword(int num, Var* v_dst, Var* v_addr, int i_offset = 0)
		{
		}
		template <typename T>
		E_ReturnState flat_load_dword(int num, Var* v_dst, Var* v_offset, T s_addr, int i_offset = 0)
		{
			if (IsaArch >= E_IsaArch::Gfx900)
			{
				return flat_load_dword_gfx900(num, v_dst, v_offset, s_addr, i_offset);
			}
			else
			{
				return flat_load_dword_gfx800(num, v_dst, v_offset, i_offset);
			}
		}
		E_ReturnState flat_load_dword_gfx800(int num, Var* v_dst, Var* v_offset_addr, int i_offset = 0)
		{
			int tmpIdx;
			std::string str = "";
			str.append("flat_load_dword");
			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			if (num == 1)
			{
				str.append(getVar(v_dst));
			}
			else
			{
				str.append(getVar(v_dst, num));
			}
			str.append(", ");
			str.append(getVar(v_offset_addr, 2));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);

			if (!((num > 0) && (num <= 4)))
			{
				str.append("load data error");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("base addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->vgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_dst->type != E_VarType::VAR_VGPR)
			{
				str.append("store data reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (!((i_offset >= 0) && (i_offset <= 4095)))
			{
				str.append("imm_offset over 13-bit int");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		template <typename T>
		E_ReturnState flat_load_dword_gfx900(int num, Var* v_dst, Var* v_offset_addr, T s_addr, int i_offset = 0)
		{
			int tmpIdx;
			std::string str = "";
			str.append("global_load_dword");
			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			if (num == 1)
			{
				str.append(getVar(v_dst));
			}
			else
			{
				str.append(getVar(v_dst, num));
			}
			str.append(", ");
			str.append(getVar(v_offset_addr, 2));
			str.append(", ");
			str.append(getVar(s_addr));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);

			if (!((num > 0) && (num <= 4)))
			{
				str.append("load data error");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("base addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->vgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_dst->type != E_VarType::VAR_VGPR)
			{
				str.append("store data reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if ((getVarType(s_addr) != E_VarType::VAR_SGPR) && (getVarType(s_addr) != E_VarType::VAR_OFF))
			{
				str.append("offset reg not sgpr nor off");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (!((i_offset >= -4096) && (i_offset <= 4095)))
			{
				str.append("imm_offset over 13-bit int");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}

		template <typename T>
		E_ReturnState flat_store_dword(int num, Var* v_offset_addr, Var* v_dat, T s_addr, int i_offset = 0)
		{
			if (IsaArch >= E_IsaArch::Gfx900)
			{
				return flat_store_dword_gfx900(num, v_offset_addr, v_dat, s_addr, i_offset);
			}
			else
			{
				return flat_store_dword_gfx800(num, v_offset_addr, v_dat, i_offset);
			}
		}
		E_ReturnState flat_store_dword_gfx800(int num, Var* v_offset_addr, Var* v_dat, int i_offset = 0)
		{
			int tmpIdx;
			std::string str = "";
			str.append("flat_store_dword");

			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(v_offset_addr, 2));
			str.append(", ");
			if (num == 1)
			{
				str.append(getVar(v_dat));
			}
			else
			{
				str.append(getVar(v_dat, num));
			}

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);

			if (!((num > 0) && (num <= 4)))
			{
				str.append("load data error");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("base addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->vgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_dat->type != E_VarType::VAR_VGPR)
			{
				str.append("store data reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (!((i_offset >= 0) && (i_offset <= 4095)))
			{
				str.append("imm_offset over 13-bit int");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		template <typename T>
		E_ReturnState flat_store_dword_gfx900(int num, Var* v_offset_addr, Var* v_dat, T s_addr, int i_offset = 0)
		{
			int tmpIdx;
			std::string str = "";
			str.append("global_store_dword");

			if (num > 1)
			{
				str.append("x" + d2s(num));
			}
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(v_offset_addr, 2));
			str.append(", ");
			if (num == 1)
			{
				str.append(getVar(v_dat));
			}
			else
			{
				str.append(getVar(v_dat, num));
			}
			str.append(", ");
			str.append(getVar(s_addr));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);

			if (!((num > 0) && (num <= 4)))
			{
				str.append("load data error");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("base addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_offset_addr->vgpr.len != 2)
			{
				str.append("base addr reg not 64-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_dat->type != E_VarType::VAR_VGPR)
			{
				str.append("store data reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if ((getVarType(s_addr) != E_VarType::VAR_SGPR) && (getVarType(s_addr) != E_VarType::VAR_OFF))
			{
				str.append("offset reg not sgpr nor off");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (!((i_offset >= -4096) && (i_offset <= 4095)))
			{
				str.append("imm_offset over 13-bit int");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}

		/************************************************************************************/
		/* DS																				*/
		/************************************************************************************/
		E_ReturnState ds_read_dword(int num, Var* v_dst, Var* v_addr, int i_offset = 0, bool setM0 = false)
		{
			if (setM0 == true)
			{
				op2("s_mov_b32", "m0", -1);
			}

			int tmpIdx;
			std::string str = "";
			str.append("ds_read_b");

			str.append(d2s(num*32));
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(v_dst, num));
			str.append(", ");
			str.append(getVar(v_addr));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);

			// error check
			if (v_dst->type != E_VarType::VAR_VGPR)
			{
				str.append("dest reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("ds addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}
		E_ReturnState ds_write_dword(int num, Var* v_addr, Var* v_dat, int i_offset = 0, bool setM0 = false)
		{
			if (setM0 == true)
			{
				op2("s_mov_b32", "m0", -1);
			}

			int tmpIdx;
			std::string str = "";
			str.append("ds_write_b");

			str.append(d2s(num * 32));
			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(v_addr));
			str.append(", ");
			str.append(getVar(v_dat, num));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);
			
			// error check
			if (v_addr->type != E_VarType::VAR_VGPR)
			{
				str.append("ds addr reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			if (v_dat->type != E_VarType::VAR_VGPR)
			{
				str.append("data reg not vgpr");
				wrLine(str);
				return E_ReturnState::FAIL;
			}

			return E_ReturnState::SUCCESS;
		}

		/************************************************************************************/
		/* 通用操作																			*/
		/************************************************************************************/
		void op0(std::string op)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			wrLine(str);
		}
		template <typename T1>
		void op1(std::string op, T1 dst, int i_offset = 0)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(dst));

			if (i_offset != 0)
			{
				tmpIdx = FLAG_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");
				str.append("offset:");
				str.append(getVar(i_offset));
			}

			wrLine(str);
		}
		template <typename T1, typename T2>
		void op2(std::string op, T1 dst, T2 src)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			if ((op[op.length() - 2] == '6') && (op[op.length() - 1] == '4'))
				str.append(getVar(dst, 2));
			else
				str.append(getVar(dst));

			str.append(", ");
			if ((op[op.length() - 2] == '6') && (op[op.length() - 1] == '4'))
				str.append(getVar(src, 2));
			else
				str.append(getVar(src));

			wrLine(str);
		}
		template <typename T1, typename T2, typename T3>
		void op3(std::string op, T1 dst, T2 src0, T3 src1)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(dst));

			str.append(", ");
			str.append(getVar(src0));
			str.append(", ");
			str.append(getVar(src1));

			wrLine(str);
		}
		template <typename T1, typename T2, typename T3, typename T4>
		void op4(std::string op, T1 dst, T2 src0, T3 src1, T4 src2)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(dst));

			str.append(", ");
			str.append(getVar(src0));
			str.append(", ");
			str.append(getVar(src1));
			str.append(", ");
			str.append(getVar(src2));

			wrLine(str);
		}
		template <typename T1, typename T2, typename T3, typename T4, typename T5>
		void op5(std::string op, T1 dst, T2 src0, T3 src1, T4 src2, T5 src3)
		{
			int tmpIdx;
			std::string str = "";
			str.append(op);

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append(getVar(dst));

			str.append(", ");
			str.append(getVar(src0));
			str.append(", ");
			str.append(getVar(src1));
			str.append(", ");
			str.append(getVar(src2));
			str.append(", ");
			str.append(getVar(src3));

			wrLine(str);
		}

		E_ReturnState s_wait_lgkmcnt(uint cnt)
		{
			int tmpIdx;

			std::string str = "";
			str.append("s_waitcnt");

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append("lgkmcnt");
			str.append("(" + d2s(cnt) + ")");

			wrLine(str);

			// error check
			str = "";
			if (!((cnt >= 0) && (cnt <= 15)))
			{
				str.append("lgkmcnt is over 4-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			return E_ReturnState::SUCCESS;
		}
		E_ReturnState s_wait_vmcnt(uint cnt)
		{
			int tmpIdx;

			std::string str = "";
			str.append("s_waitcnt");

			tmpIdx = PARAM_START_COL - str.length();
			for (int i = 0; i < tmpIdx; i++)
				str.append(" ");

			str.append("vmcnt");
			str.append("(" + d2s(cnt) + ")");

			wrLine(str);

			// error check
			str = "";
			if (!((cnt >= 0) && (cnt <= 63)))
			{
				str.append("vmcnt is over 6-bit");
				wrLine(str);
				return E_ReturnState::FAIL;
			}
			return E_ReturnState::SUCCESS;
		}
#pragma endregion

#pragma region GAS_REGION
		/************************************************************************/
		/* if...else...															*/
		/************************************************************************/
		void sIF(std::string param1, std::string op, std::string param2)
		{
			std::string str = sblk();
			str.append(".if (");
			str.append(param1);
			str.append(" " + op + " ");
			str.append(param2);
			str.append(")");

			indent();
			str.append("\n");
			KernelString.append(str);

		}
		void sELSE()
		{
			backSpace();
			std::string str = sblk();
			str.append(".else");
			indent();
			str.append("\n");
			KernelString.append(str);
		}
		void eIF()
		{
			backSpace();
			std::string str = sblk();
			str.append(".endif");
			str.append("\n");
			KernelString.append(str);
		}

		/************************************************************************/
		/* for 循环																*/
		/************************************************************************/
		void sFOR(int loop)
		{
			std::string str = sblk();
			str.append(".rept(");
			str.append(d2s(loop));
			str.append(")");
			indent();
			str.append("\n");
			KernelString.append(str);
		}
		void eFOR()
		{
			backSpace();
			std::string str = sblk();
			str.append(".endr");
			str.append("\n");
			KernelString.append(str);
		}

		/************************************************************************/
		/* 定义函数																*/
		/************************************************************************/
		void sFUNC(std::string name, int parCnt, ...)
		{
			int tmpIdx;
			std::string str = sblk();
			str.append(".macro ");
			str.append(name);

			if (parCnt > 0)
			{
				tmpIdx = PARAM_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");

				va_list args;
				char * arg;
				va_start(args, parCnt);

				for (int i = 0; i < parCnt - 1; i++)
				{
					arg = va_arg(args, char*);
					str.append(arg);
					str.append(", ");
				}
				arg = va_arg(args, char*);
				str.append(arg);

				va_end(args);
			}

			indent();
			str.append("\n");
			KernelString.append(str);
		}
		void eFUNC()
		{
			backSpace();
			std::string str = sblk();
			str.append(".endm\n");
			str.append("\n");
			KernelString.append(str);
		}
		void FUNC(std::string func, int parCnt, ...)
		{
			int tmpIdx;

			std::string str = sblk();
			str.append(func);

			if (parCnt > 0)
			{
				tmpIdx = PARAM_START_COL - str.length();
				for (int i = 0; i < tmpIdx; i++)
					str.append(" ");

				va_list args;
				char * arg;
				va_start(args, parCnt);

				for (int i = 0; i < parCnt - 1; i++)
				{
					arg = va_arg(args, char*);
					str.append(arg);
					str.append(", ");
				}
				arg = va_arg(args, char*);
				str.append(arg);

				va_end(args);
			}

			str.append("\n");
			KernelString.append(str);
		}

		/************************************************************************/
		/* 变量设置																*/
		/************************************************************************/
		void refGPR(std::string tgpr, std::string gpr)
		{
			std::string str = sblk();
			str.append(tgpr);
			str.append(" = \\");
			str.append(gpr);
			str.append("\n");
			KernelString.append(str);
		}
		void setGPR(std::string tgpr, int idx)
		{
			std::string str = sblk();
			str.append(tgpr);
			str.append(" = ");
			str.append(d2s(idx));
			str.append("\n");
			KernelString.append(str);
		}
#pragma endregion
	};
}

