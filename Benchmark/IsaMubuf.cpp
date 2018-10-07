
#include "IsaMubuf.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState MubufSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtMubufSolutionConfig * extSol;

	extSol = new T_ExtMubufSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState MubufSolution::InitDev()
{
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

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
E_ReturnState MubufSolution::GetBackResult()
{
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->VectorSize * sizeof(float));
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void MubufSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                         */
/************************************************************************/
E_ReturnState MubufSolution::GenerateSolution()
{
	T_ExtMubufProblemConfig * extProb = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
	T_ExtMubufSolutionConfig * extSol = (T_ExtMubufSolutionConfig *)SolutionConfig->extConfig;

	// ======================================================================
	// 生成worksize
	// ======================================================================
	SolutionConfig->l_wk0 = WAVE_SIZE;
	SolutionConfig->l_wk1 = 1;
	SolutionConfig->l_wk2 = 1;
	SolutionConfig->g_wk0 = extProb->VectorSize;
	SolutionConfig->g_wk1 = 1;
	SolutionConfig->g_wk2 = 1;

	SolutionConfig->g_wk0 = WAVE_SIZE;

	// ======================================================================
	// 生成代码
	// ======================================================================
	SolutionConfig->KernelName = "IsaMubuf";
	SolutionConfig->KernelFile = "IsaMubufAutoGen.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterIsaMubuf * kw = new KernelWriterIsaMubuf(ProblemConfig, SolutionConfig);
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
E_ReturnState MubufProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtMubufProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtMubufProblemConfig();
	extProblemConfig->VectorSize = 1024;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "512";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState MubufProblem::InitHost()
{
	std::cout << "Mubuf Instruction init" << ProblemConfig->ConfigName << std::endl;
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

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
		exCfg->h_b[i] = i * 0.001;
		exCfg->h_c[i] = 0;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState MubufProblem::Host()
{
	printf("Mubuf instruction host.\n");
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < exCfg->VectorSize; i++)
	{
		exCfg->c_ref[i] = exCfg->h_a[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState MubufProblem::Verify()
{
	printf("Mubuf instruction verify.\n");
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;
		
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
void MubufProblem::ReleaseHost()
{
	printf("Mubuf instruction destroy.\n");
	T_ExtMubufProblemConfig * exCfg = (T_ExtMubufProblemConfig *)ProblemConfig->extConfig;

	HstFree(exCfg->h_a);
	HstFree(exCfg->h_b);
	HstFree(exCfg->h_c);
	HstFree(exCfg->c_ref);
}
#pragma endregion
