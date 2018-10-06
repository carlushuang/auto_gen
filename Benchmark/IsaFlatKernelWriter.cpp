
#include "IsaFlatKernelWriter.h"

using namespace krnelWriter;

#define FLAT_TEST		2

KernelWriterIsaFlat::KernelWriterIsaFlat(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtFlatProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtFlatSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterIsaFlat::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_b = newSgpr("s_ptr_b", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);

	v_a_addr = newVgpr("v_a_addr", 2, 2);
	v_b_addr = newVgpr("v_b_addr", 2, 2);
	v_c_addr = newVgpr("v_c_addr", 2, 2);

	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	f_linear_addr(s_ptr_a, v_a_addr);
	f_linear_addr(s_ptr_b, v_b_addr);
	f_linear_addr(s_ptr_c, v_c_addr);

	/************************************************************************/
	/* vmem读写																*/
	/* ==================================================================== */
	/* global_load_															*/
	/* global_store_														*/
	/* ==================================================================== */
	/* 例:																	*/
	/* c = a																*/
	/************************************************************************/
#if FLAT_TEST == 1
	flat_load_dword(1, v_tmp1, v_a_addr, "off");
	s_wait_vmcnt(0);
	flat_store_dword(1, v_c_addr, v_tmp1, "off");
#endif

	/************************************************************************/
	/* 原子操作																*/
	/* ==================================================================== */
	/* 算数运算:	----------------------------------------------------------- */
	/* global_atomic_add													*/
	/* global_atomic_inc													*/
	/* global_atomic_sub													*/
	/* global_atomic_dec													*/
	/* 逻辑运算:	----------------------------------------------------------- */
	/* global_atomic_and													*/
	/* global_atomic_or														*/
	/* global_atomic_xor													*/
	/* 比较运算:	----------------------------------------------------------- */
	/* global_atomic_smax													*/
	/* global_atomic_umax													*/
	/* global_atomic_smin													*/
	/* global_atomic_umin													*/
	/* global_atomic_swap													*/
	/* global_atomic_cmpswap												*/
	/* ==================================================================== */
	/* 例:																	*/
	/* c[] = tid_x															*/
	/* if(c[] < 5) {c[] = 25}												*/
	/************************************************************************/
#if FLAT_TEST == 2
	op2("v_mov_b32", v_tmp1, v_tid_x);
	flat_store_dword(1, v_c_addr, v_tmp1, "off");
	s_wait_vmcnt(0);

	op2("v_mov_b32", v_tmp2, 25);
	flat_atomic_op(OP_UMAX, v_tmp1, v_c_addr, v_tmp2, "off");
	s_wait_vmcnt(0);
#endif

	delVar(v_tmp1);
}
