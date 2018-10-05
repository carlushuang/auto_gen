
#include "IsaFlatKernelWriter.h"

using namespace krnelWriter;

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

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	f_linear_addr(s_ptr_a, v_a_addr);
	f_linear_addr(s_ptr_b, v_b_addr);
	f_linear_addr(s_ptr_c, v_c_addr);

	flat_load_dword(1, v_tmp1, v_a_addr, "off");
	s_wait_vmcnt(0);
	flat_store_dword(1, v_c_addr, v_tmp1, "off");

	delVar(v_tmp1);
}
