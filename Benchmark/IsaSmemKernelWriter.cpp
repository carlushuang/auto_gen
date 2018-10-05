
#include "IsaSmemKernelWriter.h"

using namespace krnelWriter;

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

	Var * s_tmp1 = newSgpr("s_tmp1");
	Var * s_tmp2 = newSgpr("s_tmp2");

	s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
	s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
	s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
	s_wait_lgkmcnt(0);

	s_load_dword(1, s_tmp1, s_ptr_a, 0x00);
	s_load_dword(1, s_tmp2, s_ptr_b, 0x00);
	s_wait_lgkmcnt(0);

	s_store_dword(1, s_tmp1, s_ptr_c, 0);
	s_store_dword(1, s_tmp2, s_ptr_c, 0x04);
	s_wait_lgkmcnt(0);
	//isa->inst0("s_dcache_wb","");

}
