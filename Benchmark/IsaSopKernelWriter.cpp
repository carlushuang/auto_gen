
#include "IsaSopKernelWriter.h"

using namespace krnelWriter;

#define	SOP_TEST		5

KernelWriterIsaSop::KernelWriterIsaSop(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtSopProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtSopSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterIsaSop::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_b = newSgpr("s_ptr_b", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);

	Var * s_addr_a = newSgpr("s_addr_a", 2, 2);
	Var * s_addr_b = newSgpr("s_addr_b", 2, 2);
	Var * s_addr_c = newSgpr("s_addr_c", 2, 2);

	Var * s_tmp0 = newSgpr("s_tmp0", 2, 2);
	Var * s_tmp1 = newSgpr("s_tmp1", 2, 2);
	Var * s_tmp2 = newSgpr("s_tmp2", 2, 2);
	Var * s_tmp3 = newSgpr("s_tmp2", 2, 2);

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(groupSize0) + 2);
	op3("s_add_u32", s_addr_a, s_ptr_a, s_tmp1);
	op3("s_addc_u32", *s_addr_a + 1, *s_ptr_a + 1, 0);

	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(groupSize0) + 2);
	op3("s_add_u32", s_addr_c, s_ptr_c, s_tmp1);
	op3("s_addc_u32", *s_addr_c + 1, *s_ptr_c + 1, 0);

	/************************************************************************/
	/* 算数运算																*/
	/* ==================================================================== */
	/* 赋值 --------------------------------------------------------------- */
	/* s_mov_																*/
	/* s_mov_fed_b32														*/
	/* s_movreld_															*/
	/* s_cmov_																*/
	/* s_cmovk_i32															*/
	/* s_movk_i32															*/
	/* s_cselect_															*/
	/* 算数计算------------------------------------------------------------- */
	/* s_sext_																*/
	/* s_abs_i32															*/
	/* s_absdiff_i32														*/
	/* s_add_																*/
	/* s_addc_																*/
	/* s_addk_																*/
	/* s_sub_																*/
	/* s_subb_																*/
	/* s_mul_																*/
	/* s_mulk_																*/
	/* 逻辑运算------------------------------------------------------------- */
	/* s_not_																*/
	/* s_and_																*/
	/* s_andn2_																*/
	/* s_nand_																*/
	/* s_nor_																*/
	/* s_or_																*/
	/* s_orn2_																*/
	/* s_xor_																*/
	/* s_xnor_																*/
	/* s_ashr_																*/
	/* s_bfe_																*/
	/* s_lshl1/2/3/4_add_													*/
	/* s_lshl/r_															*/
	/* 比较运算------------------------------------------------------------- */
	/* s_max_																*/
	/* s_min_																*/
	/* s_cmp(k)_eq_															*/
	/* s_cmp(k)_lg_															*/
	/* s_cmp(k)_ge_															*/
	/* s_cmp(k)_gt_															*/
	/* s_cmp(k)_le_															*/
	/* s_cmp(k)_lt_															*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SOP_TEST == 1
#endif

	/************************************************************************/
	/* 位运算																*/
	/* ==================================================================== */
	/* s_bcnt0/1_															*/
	/* s_bitreplicate_b64_b32												*/
	/* s_bitset0/1_															*/
	/* s_brev_																*/
	/* s_ff0/1_																*/
	/* s_flbit_																*/
	/* s_bcnt0/1_															*/
	/* s_quadmask_															*/
	/* s_wqm_																*/
	/* s_pack_																*/
	/* s_bitcmp0/1_															*/
	/* s_bitcmp0/1_															*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SOP_TEST == 2
#endif

	/************************************************************************/
	/* EXEC操作																*/
	/* ==================================================================== */
	/* s_and_saveexec_														*/
	/* s_andn1_saveexec_													*/
	/* s_andn2_saveexec_													*/
	/* s_nand_saveexec_														*/
	/* s_nor_saveexec_														*/
	/* s_xnor_saveexec_														*/
	/* s_or_saveexec_														*/
	/* s_xor_saveexec_														*/
	/* s_orn1_saveexec_														*/
	/* s_orn2_saveexec_														*/
	/* s_xxx_wrexec_														*/	
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SOP_TEST == 3
#endif
	
	/************************************************************************/
	/* 流程控制																*/
	/* ==================================================================== */
	/* s_branch																*/
	/* s_cbranch_join														*/
	/* s_cbranch_g_fork														*/
	/* s_cbranch_i_fork														*/
	/* s_cbranch_exec(n)z													*/
	/* s_cbranch_scc0/1														*/
	/* s_cbranch_vcc(n)z													*/
	/* s_getpc_b64															*/
	/* s_setpc_b64															*/
	/* s_swappc_b64															*/
	/* s_call_b64															*/
	/* s_rfe_b64															*/
	/* s_rfe_restore_b64													*/
	/* 程序状态 ------------------------------------------------------------ */
	/* s_barrier															*/
	/* s_endpgm_															*/
	/* s_sethalt															*/
	/* s_setkill															*/
	/* s_trap																*/
	/* s_setvskip															*/
	/* s_setprio															*/
	/* s_nop																*/
	/* s_sleep																*/
	/* s_wakeup																*/
	/* debug -------------------------------------------------------------- */
	/* s_ttracedata															*/
	/* s_cbranch_cdbgsys_													*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SOP_TEST == 4
#endif


	/************************************************************************/
	/* 硬件状态控制															*/
	/* ==================================================================== */
	/* gpr ---------------------------------------------------------------- */
	/* s_set_gpr_idx_on														*/
	/* s_set_gpr_idx_idx													*/
	/* s_set_gpr_idx_mode													*/
	/* s_set_gpr_idx_off													*/
	/* debug -------------------------------------------------------------- */
	/* s_getreg_b32															*/
	/* s_setreg_															*/
	/* performance counter ------------------------------------------------ */
	/* s_decperflevel														*/
	/* s_incperflevel														*/
	/* L1 I-cache --------------------------------------------------------- */
	/* s_icache_inv															*/
	/* message ------------------------------------------------------------ */
	/* s_sendmsg															*/
	/* s_sendmsghalt														*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SOP_TEST == 5
	Var * s_wave_id = newSgpr("s_wave_id");
	Var * s_simd_id = newSgpr("s_simd_id");
	Var * s_cu_id = newSgpr("s_cu_id");
	Var * s_se_id = newSgpr("s_se_id");
	Var * s_tg_id = newSgpr("s_tg_id");
	f_read_hw_reg_hw_id(s_wave_id, s_simd_id, "off", s_cu_id, "off", s_se_id, s_tg_id, "off", "off", "off", "off");
	s_store_dword(1, s_wave_id, s_addr_c, 4*0);
	s_store_dword(1, s_simd_id, s_addr_c, 4*1);
	s_store_dword(1, s_cu_id, s_addr_c, 4*2);
	s_store_dword(1, s_se_id, s_addr_c, 4*3);
	s_store_dword(1, s_tg_id, s_addr_c, 4*4);
	s_wait_lgkmcnt(0);
	op0("s_dcache_wb");
#endif

	clrVar();
}
