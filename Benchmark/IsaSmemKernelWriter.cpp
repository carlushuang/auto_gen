
#include "IsaSmemKernelWriter.h"

using namespace krnelWriter;

#define	SMEM_TEST		5

KernelWriterIsaSmem::KernelWriterIsaSmem(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtSmemProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtSmemSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

void KernelWriterIsaSmem::writeProgram()
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
	/* smem读写																*/
	/* ==================================================================== */
	/* s_load_																*/
	/* s_store_																*/
	/* ==================================================================== */
	/* 例:																	*/
	/* 读取a数组前group_num个数												*/
	/* 存储到c数组															*/
	/************************************************************************/
#if SMEM_TEST == 1
	op3("s_lshl_b32", s_tmp0, s_gid_x, 2);
	s_load_dword(1, s_tmp1, s_addr_a, 0);
	s_wait_lgkmcnt(0);
	s_store_dword(1, s_tmp1, s_ptr_c, s_tmp0);
	s_wait_lgkmcnt(0);
	op0("s_dcache_wb");
#endif

	/************************************************************************/
	/* 原子操作																*/
	/* ==================================================================== */
	/* 算数运算:	----------------------------------------------------------- */
	/* s_atomic_add															*/
	/* s_atomic_inc															*/
	/* s_atomic_sub															*/
	/* s_atomic_dec															*/
	/* 逻辑运算:	----------------------------------------------------------- */
	/* s_atomic_and															*/
	/* s_atomic_or															*/
	/* s_atomic_xor															*/
	/* 比较运算:	----------------------------------------------------------- */
	/* s_atomic_smax														*/
	/* s_atomic_umax														*/
	/* s_atomic_smin														*/
	/* s_atomic_umin														*/
	/* s_atomic_swap														*/
	/* s_atomic_cmpswap														*/
	/* ==================================================================== */
	/* 例:																	*/
	/* c[] = gid_x															*/
	/* if(c[] < 8) {c[] = 8}												*/
	/************************************************************************/
#if SMEM_TEST == 2
	op3("s_lshl_b32", s_tmp0, s_gid_x, 2);
	s_store_dword(1, s_gid_x, s_ptr_c, s_tmp0);
	s_wait_lgkmcnt(0);
	op2("s_mov_b32", s_tmp1, 8);
	s_atomic_op(OP_UMAX, s_tmp1, s_ptr_c, s_tmp0);
	s_wait_lgkmcnt(0);
#endif

	/************************************************************************/
	/* buffer smem 读写														*/
	/* ==================================================================== */
	/* s_buffer_load_														*/
	/* s_buffer_store_														*/
	/* ==================================================================== */
	/* 例:																	*/
	/* 读取a数组前group_num个数												*/
	/* 存储到c数组															*/
	/************************************************************************/
#if SMEM_TEST == 3
#endif

	/************************************************************************/
	/* buffer 原子操作														*/
	/* ==================================================================== */
	/* 算数运算:	----------------------------------------------------------- */
	/* s_buffer_atomic_add													*/
	/* s_buffer_atomic_inc													*/
	/* s_buffer_atomic_sub													*/
	/* s_buffer_atomic_dec													*/
	/* 逻辑运算:	----------------------------------------------------------- */
	/* s_buffer_atomic_and													*/
	/* s_buffer_atomic_or													*/
	/* s_buffer_atomic_xor													*/
	/* 比较运算:	----------------------------------------------------------- */
	/* s_buffer_atomic_smax													*/
	/* s_buffer_atomic_umax													*/
	/* s_buffer_atomic_smin													*/
	/* s_buffer_atomic_umin													*/
	/* s_buffer_atomic_swap													*/
	/* s_buffer_atomic_cmpswap												*/
	/* ==================================================================== */
	/* 例:																	*/
	/* c[] = gid_x															*/
	/* if(c[] < 8) {c[] = 8}												*/
	/************************************************************************/
#if SMEM_TEST == 4
#endif

	/************************************************************************/
	/* 计数器读取															*/
	/* ==================================================================== */
	/* s_memtime															*/
	/* s_memrealtime														*/
	/* ==================================================================== */
	/* 例:																	*/
	/* 先读取两个计数器的值,存在c[0:1], c[2:3] (2-dword)						*/
	/* 读取a数组																*/
	/* 再读取两个计数器的值,存在c[4:5], c[6:7] (2-dword)						*/
	/************************************************************************/
#if SMEM_TEST == 5
	op1("s_memtime", *s_tmp1 ^ 2);
	op1("s_memrealtime", *s_tmp2 ^ 2);
	s_wait_lgkmcnt(0);
	s_store_dword(2, s_tmp1, s_ptr_c, 8*0);
	s_store_dword(2, s_tmp2, s_ptr_c, 8*1);
	s_wait_lgkmcnt(0);

	op3("s_lshl_b32", s_tmp0, s_gid_x, 2);
	s_load_dword(1, s_tmp1, s_addr_a, s_tmp0);
	s_wait_lgkmcnt(0);

	op1("s_memtime", *s_tmp1 ^ 2);
	op1("s_memrealtime", *s_tmp2 ^ 2);
	s_wait_lgkmcnt(0);
	s_store_dword(2, s_tmp1, s_ptr_c, 8*2);
	s_store_dword(2, s_tmp2, s_ptr_c, 8*3);
	s_wait_lgkmcnt(0);

	op0("s_dcache_wb");
#endif

	/************************************************************************/
	/* SQ data catch 操作													*/
	/* ==================================================================== */
	/* s_dcache_discard_													*/
	/* s_dcache_inv_														*/
	/* s_dcache_wb_															*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SMEM_TEST == 6
#endif

	/************************************************************************/
	/* ATC预取																*/
	/* ==================================================================== */
	/* s_atc_probe															*/
	/* s_atc_probe_buffer													*/
	/* ==================================================================== */
	/* 例:																	*/
	/************************************************************************/
#if SMEM_TEST == 7
#endif

	clrVar();
}
