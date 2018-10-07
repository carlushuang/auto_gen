
#include "IsaDs.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState DsSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtDsSolutionConfig * extSol;

	extSol = new T_ExtDsSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState DsSolution::InitDev()
{
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;

	DevMalloc((void**)&(d_a.ptr), exCfg->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_b.ptr), exCfg->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_c.ptr), exCfg->VectorSize * sizeof(float));

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
	d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
	d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);

	Copy2Dev((cl_mem)(d_a.ptr), exCfg->h_a, exCfg->VectorSize * sizeof(float));
	Copy2Dev((cl_mem)(d_b.ptr), exCfg->h_b, exCfg->VectorSize * sizeof(float));

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState DsSolution::GetBackResult()
{
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->VectorSize * sizeof(float));
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void DsSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                         */
/************************************************************************/
E_ReturnState DsSolution::GenerateSolution()
{
	T_ExtDsProblemConfig * extProb = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;
	T_ExtDsSolutionConfig * extSol = (T_ExtDsSolutionConfig *)SolutionConfig->extConfig;

	// ======================================================================
	// 生成worksize
	// ======================================================================
	SolutionConfig->l_wk0 = WAVE_SIZE;
	SolutionConfig->l_wk1 = 1;
	SolutionConfig->l_wk2 = 1;
	SolutionConfig->g_wk0 = extProb->VectorSize;
	SolutionConfig->g_wk1 = 1;
	SolutionConfig->g_wk2 = 1;

	// ======================================================================
	// 生成代码
	// ======================================================================
	SolutionConfig->KernelName = "IsaDs";
	SolutionConfig->KernelFile = "IsaDsAutoGen.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterIsaDs * kw = new KernelWriterIsaDs(ProblemConfig, SolutionConfig);
	kw->GenKernelString();
	kw->PrintKernelString();
	kw->SaveKernelString2File();

	return E_ReturnState::SUCCESS;
}
#pragma endregion

/************************************************************************/
/* 问题控制                                                             */
/************************************************************************/
#pragma region ProblemRegion
/************************************************************************/
/* 生成问题空间													        */
/************************************************************************/
E_ReturnState DsProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtDsProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtDsProblemConfig();
	extProblemConfig->VectorSize = 1024;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "512";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState DsProblem::InitHost()
{
	std::cout << "Ds Instruction init" << ProblemConfig->ConfigName << std::endl;
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;

	ProblemConfig->Calculation = exCfg->VectorSize; 
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

	exCfg->h_a = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
	exCfg->h_b = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
	exCfg->h_c = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
	exCfg->c_ref = (float*)HstMalloc(exCfg->VectorSize * sizeof(float));
		
	for (int i = 0; i < exCfg->VectorSize; i++)
	{
		exCfg->h_a[i] = i;
		exCfg->h_b[i] = 2;
		exCfg->h_c[i] = 0;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState DsProblem::Host()
{
	printf("Ds instruction host.\n");
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < exCfg->VectorSize; i++)
	{
		exCfg->c_ref[i] = exCfg->h_a[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState DsProblem::Verify()
{
	printf("Ds instruction verify.\n");
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;
		
	float diff = 0;
	for (int i = 0; i < exCfg->VectorSize; i++)
	{
		diff += (exCfg->c_ref[i] - exCfg->h_c[i]) * (exCfg->c_ref[i] - exCfg->h_c[i]);
	}
	diff /= exCfg->VectorSize;

	printf("mean err = %.1f.\n", diff);
	if (diff > MIN_FP32_ERR)
	{
		printf("err = %.2f\n", diff);
		INFO("verify failed!");
		return E_ReturnState::FAIL;
	}
	INFO("verify success.");
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 释放                                                                  */
/************************************************************************/
void DsProblem::ReleaseHost()
{
	printf("Ds instruction destroy.\n");
	T_ExtDsProblemConfig * exCfg = (T_ExtDsProblemConfig *)ProblemConfig->extConfig;

	HstFree(exCfg->h_a);
	HstFree(exCfg->h_b);
	HstFree(exCfg->h_c);
	HstFree(exCfg->c_ref);
}
#pragma endregion
