
#include "ProducerConsumerKernelWriter.h"

using namespace krnelWriter;

KernelWriterProducerConsumer::KernelWriterProducerConsumer(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtProducerConsumerProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtProducerConsumerSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

#define WAVE_NUM_OFFSET		(0)
#define	SIGNAL_OFFSET		(1)
#define	DEBUG_OFFSET		(SIGNAL_OFFSET + 32)
void KernelWriterProducerConsumer::writeProgram()
{
	s_ptr_a = newSgpr("s_ptr_a", 2, 2);
	s_ptr_c = newSgpr("s_ptr_c", 2, 2);
	s_ptr_sig = newSgpr("s_ptr_sig", 2, 2);

	s_a_addr = newSgpr("s_a_addr", 2, 2);
	s_c_addr = newSgpr("s_c_addr", 2, 2);
	s_cu_sig_addr = newSgpr("s_cu_sig_addr", 2, 2);

	Var * v_a = newVgpr("var_a");
	Var * v_c = newVgpr("var_c");
	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * v_tmp1 = newVgpr("v_tmp1");
	Var * v_tmp2 = newVgpr("v_tmp2");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_sig, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

//	op3("s_lshl_b32", s_tmp1, s_gid_x, log2(groupSize0) + 2);
//	op3("s_add_u32", s_a_addr, s_ptr_a, s_tmp1);
//	op3("s_addc_u32", *s_a_addr + 1, *s_ptr_a + 1, 0);
//	op3("s_add_u32", s_c_addr, s_ptr_c, s_tmp1);
//	op3("s_addc_u32", *s_c_addr + 1, *s_ptr_c + 1, 0);

	// 计算每个CU的信号槽地址
	f_signal_slot_addr(s_cu_sig_addr, s_ptr_sig, extProbCfg->SignalPerCU);
	
	// 前64个group跳转到prefetch
	Var * l_fetch = newLaber("FETCH_WAVE");
	Var * l_work = newLaber("WORK_WAVE");
	op2("s_cmp_lt_u32", s_gid_x, CU_NUM);
	op1("s_cbranch_scc0", l_work);

	// -----------------------------------------------------------------------------------------
	// prefetch group 
	wrLaber(l_fetch);

	// 等待work group 分发信号下标完成
	op1("s_sleep", 50);

	// 测试用循环计数器
	Var * s_loop_cnt1 = newSgpr("s_loop_cnt1");
	op2("s_mov_b32", s_loop_cnt1, 0);

	// prefetch loop
	Var * l_begin_fetch = newLaber("FETCH_LOOP");
	Var * l_end_fetch = newLaber("FETCH_END");
	f_start_pend_signal(s_cu_sig_addr, l_begin_fetch, l_end_fetch, WAVE_NUM_OFFSET, SIGNAL_OFFSET);

	// 模拟prefetch工作
	op3("s_add_u32", s_loop_cnt1, s_loop_cnt1, 1);

	f_end_pend_signal(l_begin_fetch, l_end_fetch);

	// 测试用
	op2("s_mov_b32", s_tmp1, 55555555);
	s_store_dword(1, s_tmp1, s_cu_sig_addr, (DEBUG_OFFSET + 0) * 4, true);
	s_store_dword(1, s_loop_cnt1, s_cu_sig_addr, (DEBUG_OFFSET + 1) * 4, true);
	s_wait_lgkmcnt(0);
	op1("s_branch", l_end_prg);

	delVar(s_loop_cnt1);

	// -----------------------------------------------------------------------------------------
	// work group 
	wrLaber(l_work);
	// 调整group_idx
	op3("s_sub_u32", s_gid_x, s_gid_x, 64);

	Var * v_signal = newVgpr("v_signal");
	Var * v_sig_addr = newVgpr("v_sig_addr", 2, 2);
	
	f_init_signal_slot(s_cu_sig_addr, v_sig_addr, WAVE_NUM_OFFSET, SIGNAL_OFFSET);

	// 计算
	Var * s_a = newSgpr("s_a");
	Var * v_sum = newVgpr("v_sum");
	Var * s_sum = newSgpr("s_sum");
	op2("v_mov_b32", v_sum, 0);
	for (int i = 0; i < 13; i++)
	{
		// 发射信号(不需要写入L2)
		op2("v_mov_b32", v_signal, i);
		op3("v_and_b32", v_signal, v_signal, 3);	// 模拟发一个信号0,1,2,3,0,1,2,3,...
		f_send_signal(v_sig_addr, v_signal, SIGNAL_OFFSET);

		for (int j = 0; j < 50; j++)
		{
			op2("v_mov_b32", v_sum, 123);
		}
//		s_load_dword(1, s_a, s_a_addr, 0);
//		op3("s_add_u32", s_a_addr, s_a_addr, 4);
//		op3("s_addc_u32", *s_a_addr + 1, *s_a_addr + 1, 0);
//		s_wait_lgkmcnt(0);
//		op3("v_add_f32", v_sum, v_sum, s_a);

//		for (int j = 0; j < 64; j++)
//		{
//			op1("s_nop", 500);
//		}
//		op1("s_sleep", 1);
		op1("s_sleep", 200);			// (!!!!!!!!!!!!!!!)
	}
	op2("v_readfirstlane_b32", s_sum, v_sum);

	delVar(s_a);
	delVar(s_sum);
	delVar(v_sum);

	delVar(v_sig_addr);
	delVar(v_signal);
	delVar(v_tmp1);
	delVar(v_tmp2);
	delVar(s_tmp1);
		
	f_deinit_signal_slot(s_cu_sig_addr, WAVE_NUM_OFFSET);
}
