
#include "IsaDsKernelWriter.h"

using namespace krnelWriter;

#define DS_TEST		2

KernelWriterIsaDs::KernelWriterIsaDs(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtDsProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtDsSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterIsaDs::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_b = newSgpr("s_ptr_b", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);

	v_a_addr = newVgpr("v_a_addr", 2, 2);
	v_b_addr = newVgpr("v_b_addr", 2, 2);
	v_c_addr = newVgpr("v_c_addr", 2, 2);

	Var * v_tmp0 = newVgpr("v_tmp0");
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	f_linear_addr(s_ptr_a, v_a_addr);
	f_linear_addr(s_ptr_b, v_b_addr);
	f_linear_addr(s_ptr_c, v_c_addr);

	ldsByteCount += 4;	// ����lds�ֽ���

	/************************************************************************/
	/* lds��д																*/
	/* ==================================================================== */
	/* ds_read_																*/
	/* ds_write																*/
	/* ==================================================================== */
	/* ��:																	*/
	/* ��ȡa����,�洢��lds													*/
	/* ��lds����,�洢��c����													*/
	/************************************************************************/
#if DS_TEST == 1
	flat_load_dword(1, v_tmp1, v_a_addr, "off");
	s_wait_vmcnt(0);

	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	ds_write_dword(1, v_tmp0, v_tmp1, 0, true);
	s_wait_lgkmcnt(0);

	ds_read_dword(1, v_tmp2, v_tmp0);
	s_wait_lgkmcnt(0);
	flat_store_dword(1, v_c_addr, v_tmp2, "off");
#endif

	/************************************************************************/
	/* �������߼�����															*/
	/* ==================================================================== */
	/* ��������:	----------------------------------------------------------- */
	/* ds_add_ 																*/
	/* ds_inc_																*/
	/* ds_dec_																*/
	/* ds_sub_																*/
	/* ds_rsub_																*/
	/* �߼�����:	----------------------------------------------------------- */
	/* ds_and_																*/
	/* ds_or_																*/
	/* ds_mskor_															*/
	/* ds_xor_																*/
	/* �Ƚ�����:	----------------------------------------------------------- */
	/* ds_max_																*/
	/* ds_min_																*/
	/* �û�����:	----------------------------------------------------------- */
	/* ds_cmpst_  															*/
	/* ds_wrap_																*/
	/* ds_wrxchg_															*/
	/* ds_condxchg32_rtn_b64												*/
	/* ==================================================================== */
	/* ��:																	*/
	/* lds = tid_x															*/
	/* if(lds[tid] == 2) {lds[tid] = 1.23}									*/
	/* c =lds[]																*/
	/************************************************************************/
#if DS_TEST == 2
	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	op2("v_cvt_f32_u32", v_tmp2, v_tid_x);
	ds_write_dword(1, v_tmp0, v_tmp2, 0, true);
	s_wait_lgkmcnt(0);

	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	op2("v_mov_b32", v_tmp1, 1.23);
	op2("v_mov_b32", v_tmp2, 2);
	op2("v_cvt_f32_u32", v_tmp2, v_tmp2);
	op3("ds_cmpst_f32", v_tmp0, v_tmp2, v_tmp1);
	s_wait_lgkmcnt(0);

	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	ds_read_dword(1, v_tmp2, v_tmp0);
	s_wait_lgkmcnt(0);

	flat_store_dword(1, v_c_addr, v_tmp2, "off");
#endif

	/************************************************************************/
	/* ���ݽ���(��ռ��lds�ռ�)												*/
	/* ==================================================================== */
	/* ds_permute															*/
	/* ds_bpermute															*/
	/* ds_swizzle_															*/
	/* ==================================================================== */
	/* ��:																	*/
	/************************************************************************/
#if DS_TEST == 3
	op2("v_mov_b32", v_tmp0, 4);
	op2("v_mov_b32", v_tmp1, v_tid_x);
	op3("ds_permute_b32", v_tmp2, v_tmp0, v_tmp1);
	s_wait_lgkmcnt(0);

	op2("v_cvt_f32_u32", v_tmp2, v_tmp2);
	flat_store_dword(1, v_c_addr, v_tmp2, "off");
#endif

	/************************************************************************/
	/* ������-������															*/
	/* ==================================================================== */
	/* LDS����:	----------------------------------------------------------- */
	/* ds_append															*/
	/* ds_consume															*/
	/* GDS����:	----------------------------------------------------------- */
	/* ds_ordered_count														*/
	/* ds_gws_barrier														*/
	/* ds_gws_init															*/
	/* ds_gws_sema_br														*/
	/* ds_gws_sema_p														*/
	/* ds_gws_sema_release_all												*/
	/* ds_gws_sema_v														*/
	/* ==================================================================== */
	/* ��:																	*/
	/* lds[]����																*/
	/* exec = 32��thread														*/
	/* lds[0] += count_bits(exec_mask)										*/
	/* exec = 8��thread														*/
	/* lds[0] -= count_bits(exec_mask)										*/
	/* c = lds																*/
	/************************************************************************/
#if DS_TEST == 4
	op2("v_mov_b32", v_tmp1, 0);
	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	ds_write_dword(1, v_tmp0, v_tmp1, 0, true);

	op2("s_mov_b64", "exec", "0xFFFFFFFF");
	op2("s_mov_b32", "m0", 0);
	op1("ds_append", v_tmp1);
	s_wait_lgkmcnt(0);
	op2("s_mov_b64", "exec", "0xF");
	op1("ds_consume", v_tmp1);
	s_wait_lgkmcnt(0);
	op2("s_mov_b64", "exec", "0xFFFFFFFFFFFFFFFF");

	op3("v_lshlrev_b32", v_tmp0, 2, v_tid_x);
	ds_read_dword(1, v_tmp2, v_tmp0, 0, true);
	s_wait_lgkmcnt(0);

	op2("v_cvt_f32_u32", v_tmp2, v_tmp2);
	flat_store_dword(1, v_c_addr, v_tmp2, "off");
#endif

	delVar(v_tmp1);
}
