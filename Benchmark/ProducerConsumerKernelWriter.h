#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"

namespace AutoGen
{

	class KernelWriterProducerConsumer :public KernelWriter
	{

#define WAVE_NUM_OFFSET		(0)
#define	SIGNAL_OFFSET		(1)
#define	DEBUG_OFFSET		(SIGNAL_OFFSET + 32)

	public:
		KernelWriterProducerConsumer(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
			KernelWriter(probCfg, solCfg)
		{
			extProbCfg = (T_ExtProducerConsumerProblemConfig *)problemConfig->extConfig;
			extSolCfg = (T_ExtProducerConsumerSolutionConfig *)solutionConfig->extConfig;

			VectorSize = extProbCfg->VectorSize;
		}

	protected:
		int N = 0;
		int VectorSize;
		T_ExtProducerConsumerProblemConfig * extProbCfg;	// ��ǰ���ڴ����������չ����
		T_ExtProducerConsumerSolutionConfig * extSolCfg;	// ��ǰ���ڴ���Ľ��������չ����

		Var * s_ptr_a;
		Var * s_ptr_c;
		Var * s_ptr_sig;


		void writeMetadata()
		{
			setTable(0);
			wrLine(".amd_amdgpu_hsa_metadata");
			wrLine("{ Version: [1, 0],");
			wrLine("  Kernels :");
			wrLine("    - { Name: " + kernelName + ",");
			wrLine("        SymbolName: " + kernelName + ",");
			wrLine("        Language: OpenCL C, LanguageVersion: [ 1, 2 ],");
			wrLine("        Attrs: { ReqdWorkGroupSize: [ " + d2s(groupSize0) + ", " + d2s(groupSize1) + ", " + d2s(groupSize2) + " ] }");
			wrLine("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 512 }");
			wrLine("        Args:");
			wrLine("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
			wrLine("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
			wrLine("        - { Name: d_sig , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
			wrLine("      }");
			wrLine("}");
			wrLine(".end_amd_amdgpu_hsa_metadata");
			wrLine("");
		}

		void writeProgram()
		{
			s_ptr_a = newSgpr("s_ptr_a", 2, 2);
			s_ptr_c = newSgpr("s_ptr_c", 2, 2);
			s_ptr_sig = newSgpr("s_ptr_sig", 2, 2);

			Var * s_tmp1 = newSgpr("s_tmp1");
			Var * v_tmp1 = newVgpr("v_tmp1");

			s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
			s_load_dword(2, s_ptr_c, s_kernelArg, 0x08);
			s_load_dword(2, s_ptr_sig, s_kernelArg, 0x10);
			s_wait_lgkmcnt(0);

			// ����ÿ��CU���źŲ۵�ַ(ģ�����ݵ�ַ����)
			Var * s_a_addr = newSgpr("s_a_addr", 2, 2);
			Var * s_c_addr = newSgpr("s_c_addr", 2, 2);
			Var * s_cu_sig_addr = newSgpr("s_cu_sig_addr", 2, 2);
			f_signal_slot_addr(s_a_addr, s_ptr_a, VectorSize);
			f_signal_slot_addr(s_c_addr, s_ptr_c, 1);
			f_signal_slot_addr(s_cu_sig_addr, s_ptr_sig, extProbCfg->SignalPerCU);

			// ǰ64��group��ת��prefetch
			Var * l_fetch = newLaber("FETCH_WAVE");
			Var * l_work = newLaber("WORK_WAVE");
			op2("s_cmp_lt_u32", s_gid_x, CU_NUM);
			op1("s_cbranch_scc0", l_work);

			// =========================================================================================
			// prefetch group 
			wrLaber(l_fetch);
			//op1("s_branch", l_end_prg);

			// �ȴ�work group �ַ��ź��±����
			op1("s_sleep", 50);

			// ������ѭ��������
			Var * s_fetch_addr = newSgpr("s_fetch_addr", 2, 2);
			Var * s_signal = newSgpr("s_signal");
			Var * s_loop_cnt1 = newSgpr("s_loop_cnt1");
			op2("s_mov_b32", s_loop_cnt1, 0);

			// prefetch loop
			Var * l_begin_fetch = newLaber("FETCH_LOOP");
			Var * l_end_fetch = newLaber("FETCH_END");
			//f_s_pend_signal(s_cu_sig_addr, l_begin_fetch, l_end_fetch, WAVE_NUM_OFFSET, SIGNAL_OFFSET, s_signal);
			test_func(s_cu_sig_addr, l_begin_fetch, l_end_fetch, WAVE_NUM_OFFSET, SIGNAL_OFFSET, s_signal);

			// prefetch����
			op3("s_add_u32", s_loop_cnt1, s_loop_cnt1, 1);

			//op2("s_mov_b32", s_signal, 3);
			op3("s_lshl_b32", s_tmp1, s_signal, log2(16) + 2);
			op3("s_add_u32", s_fetch_addr, s_a_addr, s_tmp1);
			op3("s_addc_u32", *s_fetch_addr + 1, *s_a_addr + 1, 0);
			s_load_dword(1, s_tmp1, s_fetch_addr, 0);
			s_store_dword(1, s_signal, s_cu_sig_addr, (DEBUG_OFFSET + 3) * 4, true);
			s_wait_lgkmcnt(0);

			f_e_pend_signal(l_begin_fetch, l_end_fetch);

			// ������
			op2("s_mov_b32", s_tmp1, 55555555);
			//op2("s_mov_b32", s_tmp1, "0x55555555");
			s_store_dword(1, s_tmp1, s_cu_sig_addr, (DEBUG_OFFSET + 0) * 4, true);
			s_store_dword(1, s_loop_cnt1, s_cu_sig_addr, (DEBUG_OFFSET + 1) * 4, true);
			s_wait_lgkmcnt(0);
			op1("s_branch", l_end_prg);

			delVar(s_loop_cnt1);
			delVar(s_signal);
			delVar(s_fetch_addr);

			// =========================================================================================
			// work group 
			wrLaber(l_work);
			// ����group_idx
			op3("s_sub_u32", s_gid_x, s_gid_x, 64);

			// ����ÿ��WAVE���źŵ�ַ
			Var * v_sig_addr = newVgpr("v_sig_addr", 2, 2);
			f_init_signal_slot(s_cu_sig_addr, v_sig_addr, WAVE_NUM_OFFSET, SIGNAL_OFFSET);

			// ����
			Var * s_loop_cnt = newSgpr("s_loop_cnt");
			Var * v_signal = newVgpr("v_signal");
			Var * v_sum = newVgpr("v_sum");

			op2("v_mov_b32", v_sum, 0);
			op2("v_mov_b32", v_signal, 1);
			f_s_loop(s_loop_cnt, 32, "WORK_LOOP");
			op0("s_barrier");

			// ģ����������
			for (int i = 0; i < 50; i++)
				op3("v_add_u32", v_tmp1, v_tmp1, 1);

			// �����ź�(����Ҫд��L2)
			flat_store_dword(1, v_sig_addr, v_signal, "off", SIGNAL_OFFSET * 4);
			//f_send_signal(v_sig_addr, v_signal, SIGNAL_OFFSET);
			op3("v_add_u32", v_signal, v_signal, 1);

			// ģ����������
			for (int i = 0; i < 50; i++)
				op3("v_add_u32", v_tmp1, v_tmp1, 1);


			for (int i = 0; i < 16; i++)
			{
				s_load_dword(1, s_tmp1, s_a_addr, 0);
				s_wait_lgkmcnt(0);
				op2("v_mov_b32", v_tmp1, s_tmp1);
				op3("v_add_f32", v_sum, v_sum, v_tmp1);
				// ��ַ�ۼ�
				op3("s_add_u32", s_a_addr, s_a_addr, 4);
				op3("s_addc_u32", *s_a_addr + 1, *s_a_addr + 1, 0);
			}

			op0("s_barrier");
			//op1("s_sleep", 10);// (!!!!!!!!!!!!!!!)
			f_e_loop(s_loop_cnt, "WORK_LOOP");

			op2("v_readfirstlane_b32", s_tmp1, v_sum);
			s_store_dword(1, s_tmp1, s_c_addr, 0, true);
			s_wait_lgkmcnt(0);

			f_deinit_signal_slot(s_cu_sig_addr, WAVE_NUM_OFFSET);

			delVar(v_sig_addr);
			delVar(v_signal);
			delVar(v_sum);

			delVar(v_tmp1);
			delVar(s_tmp1);

			delVar(s_a_addr);
			delVar(s_c_addr);
			delVar(s_cu_sig_addr);
		}
		
		void test_func(Var * s_signal_slot_addr,
			Var * l_begin_loop, Var * l_end_loop,
			uint wave_num_offset, uint signal_offset,
			Var * s_signal)
		{
			Var * v_tmp1 = newVgpr("v_tmp1");
			Var * v_tmp2 = newVgpr("v_tmp2");
			Var * v_signal = newVgpr("v_signal");
			Var * v_signal_addr = newVgpr("v_signal_addr", 2, 2);
			Var * v_fetch_flag = newVgpr("v_fetch_flag");
			Var * v_fetch_idx_stack = newVgpr("v_fetch_idx_stack");
			Var * s_exec_save = newSgpr("s_exec_save");
			Var * s_wave_num = newSgpr("s_wave_num");
			Var * s_old_fetch = newSgpr("s_old_fetch");
			Var * s_new_fetch = newSgpr("s_new_fetch");
			Var * s_fetched_data_flag = newSgpr("s_fetched_data_flag");

			op3("v_lshlrev_b32", v_tmp1, 2, v_tid_x);
			op2("v_mov_b32", v_tmp2, *s_signal_slot_addr + 1);
			op4("v_add_co_u32", v_signal_addr, "vcc", s_signal_slot_addr, v_tmp1);
			op5("v_addc_co_u32", *v_signal_addr + 1, "vcc", 0, v_tmp2, "vcc");

			// ������32��thread,��ֻ��32��wave��prefetch
			op2("s_mov_b32", "exec_hi", 0);

			op2("s_mov_b32", s_exec_save, "exec_lo");
			op2("v_mov_b32", v_fetch_idx_stack, 0);						// fetch��Ŷ�ջ����
			op2("s_mov_b32", s_fetched_data_flag, 0);					// ���д��ڵ�fetch��־λ����

			wrLaber(l_begin_loop);
			op2("s_mov_b32", "exec_lo", s_exec_save);
			//op3("s_add_u32", s_loop_cnt2, s_loop_cnt2, 1);
			// ʹfetch�̲߳�Ƶ������
			op1("s_sleep", 2);

			// ��ȡwave��(���atomic���ı�SQC,����Ҫ��L2��ȡ)(δ��ȫ����)
			s_load_dword(1, s_wave_num, s_signal_slot_addr, wave_num_offset * 4);
			//s_load_dword(1, s_wave_num, s_signal_slot_addr, wave_num_offset * 4, true);
			s_wait_lgkmcnt(0);

			// ����wave������0���˳�prefetch
			op2("s_cmp_eq_u32", s_wave_num, 0);
			op1("s_cbranch_scc1", l_end_loop);

			// ��ȡ�ź�(���)
			flat_load_dword(1, v_signal, v_signal_addr, "off", signal_offset * 4);
			s_wait_vmcnt(0);

			op3("v_lshlrev_b32", v_fetch_flag, v_signal, 1);			// �����ת��Ϊλ��־λ
			op2("s_mov_b32", s_exec_save, "exec_lo");					// ����exec
			op2("v_readfirstlane_b32", s_old_fetch, v_fetch_idx_stack);	// ��������fetche�����

			// �ж��յ���Ԥȡ����Ƿ�������Ŷ�ջ��
			op3("s_or_b32", s_fetched_data_flag, s_fetched_data_flag, 1);
			op3("v_xor_b32", v_tmp1, s_fetched_data_flag, v_fetch_flag);
			op3("v_and_b32", v_tmp1, v_tmp1, v_fetch_flag);
			op3("v_cmpx_ne_u32", "vcc", v_tmp1, 0);						// �ж��Ƿ����µ���Ҫfetche�ı�־λ

			// if vcc == 0 : continue
			op2("s_cmp_eq_u32", "vcc_lo", 0);
			op1("s_cbranch_scc1", l_begin_loop);

			op2("v_readfirstlane_b32", s_new_fetch, v_signal);			// �����Ҫfetch�ĵ�һ�����

			// �˴��Ѿ���õ���Ҫfetch���±꣬�����ڴ˽���fetch
			// ��Ϊ��֤�����࣬�Խ�������fetch�����������

			// �Դ��ڵ�����fetch��־λ����
			op2("s_bitset0_b32", s_fetched_data_flag, s_old_fetch);
			op2("s_bitset1_b32", s_fetched_data_flag, s_new_fetch);
			op2("s_mov_b32", s_signal, s_new_fetch);

			// �Ѿ�fetch���±�Ķ�ջ������λ����
			op2("s_mov_b32", "exec_lo", s_exec_save);
			op3("v_add_u32", v_tmp1, v_tid_x, 1);
			op3("v_lshlrev_b32", v_tmp1, 2, v_tmp1);
			op3("ds_bpermute_b32", v_fetch_idx_stack, v_tmp1, v_fetch_idx_stack);
			// д�����µ�fetch�����(ֻ����8��fetch�����)
			op3("v_writelane_b32", v_fetch_idx_stack, s_new_fetch, 7);


			s_store_dword(1, s_fetched_data_flag, s_signal_slot_addr, (DEBUG_OFFSET + 3) * 4, true);
			s_wait_lgkmcnt(0);


			delVar(v_tmp1);
			delVar(v_tmp2);
			delVar(v_signal);
			delVar(v_signal_addr);
			delVar(v_fetch_flag);
			delVar(v_fetch_idx_stack);
			delVar(s_exec_save);
			delVar(s_wave_num);
			delVar(s_old_fetch);
			delVar(s_new_fetch);
			delVar(s_fetched_data_flag);
		}
	};
}
