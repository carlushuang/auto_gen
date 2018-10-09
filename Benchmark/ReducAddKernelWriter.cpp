
#include "ReducAddKernelWriter.h"

using namespace krnelWriter;

#define RDC_ADD_TEST		1

KernelWriterReducAdd::KernelWriterReducAdd(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtReducAddProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtReducAddSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterReducAdd::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);

	v_a_addr = newVgpr("v_a_addr", 2, 2);
	v_c_addr = newVgpr("v_c_addr", 2, 2);

	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_sum = newVgpr("v_sum");
	Var * v_a = newVgpr("v_a");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	f_linear_addr(s_ptr_a, v_a_addr);
	f_linear_addr(s_ptr_c, v_c_addr);

	if (extSolCfg->Methord == 1)
	{
		// 只保留group0,以方便性能分析
		op2("s_cmp_eq_u32", s_gid_x, 0);
		op1("s_cbranch_scc0", END_PROG);

		op2("v_mov_b32", v_sum, 0);
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize * 4);

		Var * s_loop_cnt = newSgpr("s_loop_cnt");
		f_s_loop(s_loop_cnt, extProbCfg->ReducSize, "ADD_LOOP");
		flat_load_dword(1, v_a, v_a_addr, "off");
		op4("v_add_co_u32", v_a_addr, "vcc", v_a_addr, v_tmp1);
		op5("v_addc_co_u32", *v_a_addr + 1, "vcc", 0, *v_a_addr + 1, "vcc");
		s_wait_vmcnt(0);
		op3("v_add_f32", v_sum, v_sum, v_a);
		f_e_loop(s_loop_cnt, "ADD_LOOP");

		flat_store_dword(1, v_c_addr, v_sum, "off");
	}
	else if (extSolCfg->Methord == 2)
	{
		// 只保留group_x0上的wave,以方便性能分析
		op2("s_cmp_eq_u32", s_gid_x, 0);
		op1("s_cbranch_scc0", END_PROG);

		// 计算input data地址和lds地址
		// a_addr = VectoreSize * Tile * tid_y + tid_x
		// lds_addr = VectorSize * tid_y + tid_x
		Var * v_ds_addr = newVgpr("v_ds_addr"); 
		Var * s_loop_cnt;
		ldsByteCount += 64 * extSolCfg->TileGroup * 4;	// 申请lds字节数
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize * extSolCfg->Tile * 4);
		op3("v_mul_u32_u24", v_tmp1, v_tmp1, v_tid_y);
		f_addr_add_byte(v_a_addr, v_tmp1);
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize);
		op3("v_mul_u32_u24", v_ds_addr, v_tmp1, v_tid_y);
		op3("v_add_u32", v_ds_addr, v_ds_addr, v_tid_x);
		op3("v_mul_u32_u24", v_ds_addr, v_ds_addr, 4);

		op2("v_mov_b32", v_sum, 0);
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize * 4);

		// for(TileGroup) {v_sum += v_a;}
		s_loop_cnt = newSgpr("s_loop_cnt");
		f_s_loop(s_loop_cnt, extSolCfg->Tile, "ADD_LOOP");
		flat_load_dword(1, v_a, v_a_addr, "off");
		f_addr_add_byte(v_a_addr, v_tmp1);
		s_wait_vmcnt(0);
		op3("v_add_f32", v_sum, v_sum, v_a);
		f_e_loop(s_loop_cnt, "ADD_LOOP");

		// ds[] = v_sum
		ds_write_dword(1, v_ds_addr, v_sum, 0, true);
		s_wait_lgkmcnt(0);

		// wave0做累加
		op0("s_barrier");
		op3("v_cmpx_eq_u32", "vcc", v_tid_y, 0);

		// ds_addr_offset = VectorSize
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize);
		op3("v_mul_u32_u24", v_tmp1, v_tmp1, 4);

		op2("v_mov_b32", v_sum, 0);
		s_loop_cnt = newSgpr("s_loop_cnt");
		f_s_loop(s_loop_cnt, extSolCfg->TileGroup, "SUM_LOOP");
		ds_read_dword(1, v_a, v_ds_addr);
		op3("v_add_u32", v_ds_addr, v_ds_addr, v_tmp1);
		s_wait_lgkmcnt(0);
		op3("v_add_f32", v_sum, v_sum, v_a);
		f_e_loop(s_loop_cnt, "SUM_LOOP");

		flat_store_dword(1, v_c_addr, v_sum, "off");
	}

	delVar(v_tmp1);
}
