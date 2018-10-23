/*
 * GDS NOT SUPPORTED ON ROCM
*/

#include "IsaGdsKernelWriter.h"

using namespace krnelWriter;


KernelWriterIsaGds::KernelWriterIsaGds(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtGdsProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtGdsSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterIsaGds::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_b = newSgpr("s_ptr_b", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);

	v_a_addr = newVgpr("v_a_addr", 2, 2);
	v_b_addr = newVgpr("v_b_addr", 2, 2);
	v_c_addr = newVgpr("v_c_addr", 2, 2);

	Var * v_tmp0 = newVgpr("v_tmp0");
	Var * v_tmp1 = newVgpr("v_tmp1");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

//	f_linear_addr(s_ptr_a, v_a_addr);
//	f_linear_addr(s_ptr_b, v_b_addr);
	f_linear_addr(s_ptr_c, v_c_addr);


	/************************************************************************/
	/* ==================================================================== */
	/* ds_gws_init															*/
	/* ds_gws_sema_br														*/
	/* ds_gws_sema_p														*/
	/* ds_gws_sema_v														*/
	/* ds_gws_sema_release_all												*/
	/* ==================================================================== */
	/* Àý:																	*/
	/************************************************************************/
	ldsByteCount += 4;	// ÉêÇëlds×Ö½ÚÊý
	Var * v_gds_addr = newVgpr("v_gds_addr");

	op3("v_lshlrev_b32", v_gds_addr, s_gid_x, 2);
	op2("v_mov_b32", v_gds_addr, 0);

	op2("s_cmp_eq_u32", s_gid_x, 0);
	op1("s_cbranch_scc0", l_end_prg);
	op2("s_mov_b64", "exec", 1);


	op2("v_mov_b32", v_tmp0, 0);
	ds_write_dword(1, v_gds_addr, v_tmp0, 0, true, true);
	s_wait_cnt0();

	ds_read_dword(1, v_tmp1, v_gds_addr, 0, true, true);
	s_wait_cnt0();


//	op2("ds_gws_init", v_tmp0, "gds");


	op2("v_cvt_f32_u32", v_tmp1, v_tmp1);
	flat_store_dword(1, v_c_addr, v_tmp1, "off");

	delVar(v_gds_addr);

	/************************************************************************/
	/************************************************************************/

	delVar(v_tmp0);
	delVar(v_tmp1);
}
