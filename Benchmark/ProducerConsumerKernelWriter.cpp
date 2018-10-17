
#include "ProducerConsumerKernelWriter.h"

using namespace krnelWriter;

KernelWriterProducerConsumer::KernelWriterProducerConsumer(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
	KernelWriter(probCfg, solCfg)
{
	extProbCfg = (T_ExtProducerConsumerProblemConfig *)problemConfig->extConfig;
	extSolCfg = (T_ExtProducerConsumerSolutionConfig *)solutionConfig->extConfig;

	N = extProbCfg->VectorSize;
}

#define WAVE_NUM		(0)
#define	SIG_OFFSET		(1)
#define	DEBUG_OFFSET	(SIG_OFFSET + 32)
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
	Var * v_sig_addr = newVgpr("v_sig_addr", 2, 2);
	Var * v_signal = newVgpr("v_signal");
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

	// ��ȡӲ��ID
	Var * s_cu_id = newSgpr("s_cu_id");
	Var * s_se_id = newSgpr("s_se_id");
	f_read_hw_reg_hw_id("off", "off", "off", s_cu_id, "off", s_se_id, "off", "off", "off", "off", "off");
	op3("s_lshl_b32", s_tmp1, s_se_id, log2(CU_PER_SE));
	op3("s_add_u32", s_cu_id, s_tmp1, s_cu_id);

	// ����HW_CU_ID����ÿ��CU���źŲ��׵�ַ
	op3("s_lshl_b32", s_tmp1, s_cu_id, log2(extProbCfg->SignalPerCU));
	op3("s_lshl_b32", s_tmp1, s_tmp1, 2);
	op3("s_add_u32", s_cu_sig_addr, s_ptr_sig, s_tmp1);
	op3("s_addc_u32", *s_cu_sig_addr + 1, *s_ptr_sig + 1, 0);
	delVar(s_cu_id);
	delVar(s_se_id);
	
	// ǰ64��group��ת��prefetch
	Var * l_fetch = newLaber("FETCH_WAVE");
	Var * l_work = newLaber("WORK_WAVE");
	op2("s_cmp_lt_u32", s_gid_x, CU_NUM);
	op1("s_cbranch_scc0", l_work);

	// -----------------------------------------------------------------------------------------
	// prefetch group 
	wrLaber(l_fetch);

	// ������ѭ��������
	Var * s_loop_cnt1 = newSgpr("s_loop_cnt1");
	Var * s_loop_cnt2 = newSgpr("s_loop_cnt2");
	op2("s_mov_b32", s_loop_cnt1, 0);
	op2("s_mov_b32", s_loop_cnt2, 0);

	// �ȴ�work group �ַ��ź��±����
	op1("s_sleep", 50);
	op2("s_mov_b32", "exec_hi", 0);

	Var * s_wave_num = newSgpr("s_wave_num");
	Var * s_old_fetch = newSgpr("s_old_fetch");
	Var * s_new_fetch = newSgpr("s_new_fetch");
	Var * s_exec_save = newSgpr("s_exec_save");
	Var * v_fetch_idx_stack = newVgpr("v_fetch_idx_stack");
	Var * s_fetched_data_flag = newSgpr("s_fetched_data_flag");
	Var * v_fetch_flag = newVgpr("v_fetch_flag");
	op2("v_mov_b32", v_fetch_idx_stack, 0);
	op2("s_mov_b32", s_fetched_data_flag, 0);
	op2("s_mov_b32", s_exec_save, "exec_lo");

	op3("v_lshlrev_b32", v_tmp1, 2, v_tid_x);
	op2("v_mov_b32", v_tmp2, *s_cu_sig_addr + 1);
	op4("v_add_co_u32", v_sig_addr, "vcc", s_cu_sig_addr, v_tmp1);
	op5("v_addc_co_u32", *v_sig_addr + 1, "vcc", 0, v_tmp2, "vcc");


	// ------------------------------------------------------------------
	// prefetch loop
	Var * l_end_fetch = newLaber("FETCH_END");
	Var * l_fetch_loop = newLaber("FETCH_LOOP");
	wrLaber(l_fetch_loop);
	op2("s_mov_b32", "exec_lo", s_exec_save);
	op3("s_add_u32", s_loop_cnt2, s_loop_cnt2, 1);
	op1("s_sleep", 10);

	// ��ȡwave��(���atomic���ı�SQC,����Ҫ��L2��ȡ)(δ��ȫ����)
	s_load_dword(1, s_wave_num, s_cu_sig_addr, WAVE_NUM * 4);
	//s_load_dword(1, s_wave_num, s_cu_sig_addr, WAVE_NUM * 4, true);
	s_wait_lgkmcnt(0);
	
	// ����wave������0���˳�prefetch
	op2("s_cmp_eq_u32", s_wave_num, 0);
	op1("s_cbranch_scc1", l_end_fetch);

	// ��ȡ�ź�(���)
	flat_load_dword(1, v_signal, v_sig_addr, "off", SIG_OFFSET * 4);
	s_wait_vmcnt(0);	
	op3("v_lshlrev_b32", v_fetch_flag, v_signal, 1);			// �����ת��Ϊλ��־λ
	op2("s_mov_b32", s_exec_save, "exec_lo");					// ����exec
	op2("v_readfirstlane_b32", s_old_fetch, v_fetch_idx_stack);	// ��������fetche�����	
	op3("v_xor_b32", v_tmp1, s_fetched_data_flag, v_fetch_flag);
	op3("v_and_b32", v_tmp1, v_tmp1, v_fetch_flag);
	op3("v_cmpx_ne_u32", "vcc", v_tmp1, 0);						// �ж��Ƿ����µ���Ҫfetche�ı�־λ
	// if vcc == 0 : continue
	op2("s_cmp_eq_u32", "vcc_lo", 0);
	op1("s_cbranch_scc1", l_fetch_loop);

	op2("v_readfirstlane_b32", s_new_fetch, v_signal);
//	op2("s_mov_b32", s_new_fetch, 5);
	op2("s_bitset0_b32", s_fetched_data_flag, s_old_fetch);
	op2("s_bitset1_b32", s_fetched_data_flag, s_new_fetch);

	op2("s_mov_b32", "exec_lo", s_exec_save);
	op3("v_add_u32", v_tmp1, v_tid_x, 1);
	op3("v_lshlrev_b32", v_tmp1, 2, v_tmp1);
	op3("ds_bpermute_b32", v_fetch_idx_stack, v_tmp1, v_fetch_idx_stack);
	op3("v_writelane_b32", v_fetch_idx_stack, s_new_fetch, 7);	

	// ����
	op3("s_add_u32", s_loop_cnt1, s_loop_cnt1, 1);

	// end for prefetch loop
	op1("s_branch", l_fetch_loop);
	// ------------------------------------------------------------------


	// ׼���˳�prefetch �߳�(�洢���Լ�����)
	wrLaber(l_end_fetch);

	op2("s_mov_b32", s_tmp1, 55555555);
	s_store_dword(1, s_tmp1, s_cu_sig_addr, (DEBUG_OFFSET + 0) * 4, true);
	s_store_dword(1, s_old_fetch, s_cu_sig_addr, (DEBUG_OFFSET + 1) * 4, true);
	s_store_dword(1, s_new_fetch, s_cu_sig_addr, (DEBUG_OFFSET + 2) * 4, true);
	s_store_dword(1, s_fetched_data_flag, s_cu_sig_addr, (DEBUG_OFFSET + 3) * 4, true);
	s_store_dword(1, s_loop_cnt1, s_cu_sig_addr, (DEBUG_OFFSET + 4) * 4, true);
	s_store_dword(1, s_loop_cnt2, s_cu_sig_addr, (DEBUG_OFFSET + 5) * 4, true);
	s_wait_lgkmcnt(0);
	op1("s_branch", l_end_prg);

	delVar(s_loop_cnt1);
	delVar(s_loop_cnt2);
	delVar(s_fetched_data_flag);
	delVar(s_exec_save);
	delVar(v_fetch_flag);
	delVar(s_wave_num);
	
	// ������
	op2("s_mov_b64", "exec", "0xFFFF");
	flat_store_dword(1, v_sig_addr, v_signal, "off", (DEBUG_OFFSET - SIG_OFFSET) * 4);
	s_wait_vmcnt(0);
	s_store_dword(1, s_old_fetch, s_cu_sig_addr, (DEBUG_OFFSET + 0) * 4, true);
	s_wait_lgkmcnt(0);

	// -----------------------------------------------------------------------------------------
	// work group 
	wrLaber(l_work);

	// ����group_idx,����wave_id 
	Var * s_wave_id = newSgpr("s_wave_id");
	op3("s_sub_u32", s_gid_x, s_gid_x, 64);
	op3("s_lshr_b32", s_wave_id, s_gid_x, log2(CU_NUM));

	// ��ʼ��WAVE��(������ǰ��)(��Ҫд��L2����Ϊatomic����)
	Var * l_end_init = newLaber("END_INIT");
	op2("s_cmp_eq_u32", s_wave_id, 0);
	op1("s_cbranch_scc0", l_end_init);
	op2("s_mov_b32", s_tmp1, 0);
	s_store_dword(1, s_tmp1, s_cu_sig_addr, WAVE_NUM * 4, true);
	wrLaber(l_end_init);
	// �����ʼ���ȽϿ���,��Ҫ�ṩ���,�Ա�֤��ʼ�����
	op1("s_sleep", 16);
	delVar(s_wave_id);

	// ���ڲ��Եĵȴ�������ʹ��s_sleep����������
	op1("s_nop", 100);

	// ����WAVE��,��ȡ�ź��±�
	Var * s_sig_idx = newSgpr("s_sig_idx");
	op2("s_mov_b32", s_sig_idx, 1);
	s_atomic_op(E_OpType::OP_ADD, s_sig_idx, s_cu_sig_addr, WAVE_NUM * 4, true);
	s_wait_lgkmcnt(0);

	// �����ź��±�����źŵ�ַ
	Var * s_sig = newSgpr("s_sig");
	Var * s_sig_addr = newSgpr("s_sig_addr", 2, 2);
	op3("s_lshl_b32", s_sig_idx, s_sig_idx, 2);
	op3("s_add_u32", s_sig_addr, s_cu_sig_addr, s_sig_idx);
	op3("s_addc_u32", *s_sig_addr + 1, *s_cu_sig_addr + 1, 0);
	op2("v_mov_b32", v_sig_addr, s_sig_addr);
	op2("v_mov_b32", *v_sig_addr + 1, *s_sig_addr + 1);
	// ��ʼ���ź�(����Ҫд��L2)
	op2("s_mov_b32", s_sig, 0);
	s_store_dword(1, s_sig, s_sig_addr, SIG_OFFSET * 4);
	s_wait_lgkmcnt(0);
	delVar(s_sig_idx);

	// ����
	Var * s_a = newSgpr("s_a");
	Var * v_sum = newVgpr("v_sum");
	Var * s_sum = newSgpr("s_sum");
	op2("v_mov_b32", v_sum, 0);
	for (int i = 0; i < 13; i++)
	{
		// �����ź�(����Ҫд��L2)
		op2("s_mov_b32", s_sig, i);
		op3("s_and_b32", s_sig, s_sig, 3);	// ģ�ⷢһ���ź�0,1,2,3,0,1,2,3,...
		op2("v_mov_b32", v_signal, s_sig);
		flat_store_dword(1, v_sig_addr, v_signal, "off", SIG_OFFSET * 4);
		s_wait_vmcnt(0);

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
	delVar(v_sum);
	delVar(s_sig);
	delVar(s_sig_addr);
	delVar(v_sig_addr);
	delVar(v_signal);
	delVar(v_tmp1);
	delVar(v_tmp2);
	delVar(s_tmp1);
	
//	// �洢������
//	s_store_dword(1, s_sum, s_c_addr, 0, true);
//	s_wait_lgkmcnt(0);
//	delVar(s_sum);
//	
	// ����WAVE��
	op2("s_mov_b32", s_tmp1, 1);
	s_atomic_op(E_OpType::OP_SUB, s_tmp1, s_cu_sig_addr, WAVE_NUM, true);
	s_wait_lgkmcnt(0);
}
