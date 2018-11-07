#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"

namespace AutoGen
{
	class KernelWriterIsaMubuf :public KernelWriter
	{
	public:
		KernelWriterIsaMubuf(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
			KernelWriter(probCfg, solCfg)
		{
			extProbCfg = (T_ExtMubufProblemConfig *)problemConfig->extConfig;
			extSolCfg = (T_ExtMubufSolutionConfig *)solutionConfig->extConfig;

			N = extProbCfg->VectorSize;
		}

	protected:
		int N = 0;
		T_ExtMubufProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
		T_ExtMubufSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

		Var * s_ptr_a;
		Var * s_ptr_b;
		Var * s_ptr_c;

		Var * v_a_addr;
		Var * v_b_addr;
		Var * v_c_addr;

		void writeProgram()
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

			Var * s_desc = newSgpr("s_desc", 4, 4);
			Var * s_buff_offset = newSgpr("s_buff_offset");
			op3("s_lshl_b32", s_buff_offset, s_gid_x, log2(64) + 2);

			f_set_buffer_desc(s_desc, s_ptr_a, 4, 64, true);
			buffer_load_dword(1, v_tmp1, "off", s_desc, s_buff_offset, false, false, 0);
			s_wait_vmcnt(0);

			f_set_buffer_desc(s_desc, s_ptr_b, 4 * 4, 64, true, num_fmt_float, dat_fmt_32, true, idx_stride_8);
			buffer_load_dword(1, v_tmp2, "off", s_desc, s_buff_offset, false, false, 0);
			s_wait_vmcnt(0);

			op3("v_add_f32", v_tmp0, v_tmp1, v_tmp2);
			flat_store_dword(1, v_c_addr, v_tmp0, "off");

			delVar(v_tmp1);
		}
	};
}
