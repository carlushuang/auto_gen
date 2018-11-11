#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"

namespace AutoGen
{
	class KernelWriterVectAdd :public KernelWriter
	{
	public:
		KernelWriterVectAdd(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg)
			:KernelWriter(probCfg, solCfg)
		{
			extProbCfg = (T_ExtVectAddProblemConfig *)problemConfig->extConfig;
			extSolCfg = (T_ExtVectAddSolutionConfig *)solutionConfig->extConfig;

			N = extProbCfg->VectorSize;
		}

	protected:
		int N = 0;
		T_ExtVectAddProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
		T_ExtVectAddSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

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

			Var * v_a = newVgpr("var_a");
			Var * v_b = newVgpr("var_b");
			Var * v_c = newVgpr("var_c");

			s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
			s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
			s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
			s_wait_lgkmcnt(0);

			f_linear_addr(s_ptr_a, v_a_addr);
			f_linear_addr(s_ptr_b, v_b_addr);
			f_linear_addr(s_ptr_c, v_c_addr);

			flat_load_dword(1, v_a, v_a_addr, "off");
			flat_load_dword(1, v_b, v_b_addr, "off");
			s_wait_vmcnt(0);
			op3("v_add_f32", v_c, v_a, v_b);
			//op3("v_add_f32_dpp", v_c, v_a, v_b);
			flat_store_dword(1, v_c_addr, v_c, "off");

			delVar(v_a);
			delVar(v_b);
			delVar(v_c);
		}
	};
}
