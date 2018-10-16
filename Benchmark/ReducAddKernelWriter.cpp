
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
	Var * v_tmp2 = newVgpr("v_tmp2");
	Var * v_sum = newVgpr("v_sum");
	Var * v_a = newVgpr("v_a");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	f_linear_addr(s_ptr_a, v_a_addr);
	f_linear_addr(s_ptr_c, v_c_addr);

	if (extSolCfg->Methord == 1)
	{
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

		// for(Tile) {v_sum += v_a;}
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
	else if (extSolCfg->Methord == 3)
	{
		// 计算input data地址和lds地址
		// a_addr = VectoreSize * Tile * gid_x + tid_x
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize * extSolCfg->Tile);
		op3("v_mul_u32_u24", v_tmp1, v_tmp1, s_gid_x);
		op3("v_add_u32", v_tmp1, v_tmp1, v_tid_x);
		op3("v_mul_u32_u24", v_tmp1, v_tmp1, 4);
		//f_addr_add_byte(v_a_addr, v_tmp1);
		op2("v_mov_b32", v_tmp2, *s_ptr_a + 1);
		op4("v_add_co_u32", v_a_addr, "vcc", s_ptr_a, v_tmp1);
		op5("v_addc_co_u32", *v_a_addr + 1, "vcc", 0, v_tmp2, "vcc");
		// c_addr = s_ptr_c + tid_x
		op3("v_mul_u32_u24", v_tmp1, v_tid_x, 4);
		op2("v_mov_b32", v_tmp2, *s_ptr_c + 1);
		op4("v_add_co_u32", v_c_addr, "vcc", s_ptr_c, v_tmp1);
		op5("v_addc_co_u32", *v_c_addr + 1, "vcc", 0, v_tmp2, "vcc");
		
		// c[] = 0
		Var * END_INIT = newLaber("END_INIT");
		op2("s_cmp_eq_u32", s_gid_x, 0);
		op1("s_cbranch_scc0", END_INIT);
		op2("v_mov_b32", v_tmp1, 0);
		flat_store_dword(1, v_c_addr, v_tmp1, "off");
		s_wait_vmcnt(0);
		wrLaber(END_INIT);

		// for(Tile) {v_sum += v_a;}
		Var * s_loop_cnt;
		op2("v_mov_b32", v_sum, 0);
		op2("v_mov_b32", v_tmp1, extProbCfg->VectorSize * 4);

		s_loop_cnt = newSgpr("s_loop_cnt");
		f_s_loop(s_loop_cnt, extSolCfg->Tile, "ADD_LOOP");
		flat_load_dword(1, v_a, v_a_addr, "off");
		f_addr_add_byte(v_a_addr, v_tmp1);
		s_wait_vmcnt(0);
		op3("v_add_f32", v_sum, v_sum, v_a);
		f_e_loop(s_loop_cnt, "ADD_LOOP");
		delVar(v_a);
		
		// atomic add
		flat_load_dword(1, v_tmp1, v_c_addr, "off", 0, true);	// prevVal = source;			v_tmp1 = prevVal
		s_wait_vmcnt(0);
		//while(1)
		Var * ATOMIC_ADD = newLaber("ATOMIC_ADD");
		wrLaber(ATOMIC_ADD);
		op3("v_add_f32", v_tmp2, v_tmp1, v_sum);		// newVal = prevVal + operand;	v_tmp2 = newVal
		//newVal = atomic_cmpxchg(source, prevVal, newVal);								v_tmp2 = newVal
		Var * v_src_cmp = newVgpr("v_src_cmp", 2, 2);
		Var * v_rtn = newVgpr("v_rtn");
		op2("v_mov_b32", v_src_cmp, v_tmp2);
		op2("v_mov_b32", *v_src_cmp + 1, v_tmp1);
		flat_atomic_op(OP_CMPSWAP, v_rtn, v_c_addr, *v_src_cmp ^ 2, "off", 0, true);//src = DATA[0];cmp = DATA[1];
		s_wait_vmcnt(0);
		//if (newVal == prevVal)
		//	break;
		op3("v_cmpx_neq_f32", "vcc", v_tmp1, v_rtn);
		//prevVal = newVal;
		op2("v_mov_b32", v_tmp1, v_rtn);
		op1("s_cbranch_execnz", ATOMIC_ADD);
	}
	else if(extSolCfg->Methord == 4)
	{

	}

	delVar(v_tmp1);
}
