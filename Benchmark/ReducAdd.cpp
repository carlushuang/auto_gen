
#include "ReducAdd.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState ReducAddSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtReducAddSolutionConfig * extSol;

	extSol = new T_ExtReducAddSolutionConfig();
	extSol->Methord = 2;
	extSol->Tile = 10;
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState ReducAddSolution::InitDev()
{
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;

	DevMalloc((void**)&(d_a.ptr), extProb->DataSize * sizeof(float));
	DevMalloc((void**)&(d_b.ptr), extProb->DataSize * sizeof(float));
	DevMalloc((void**)&(d_c.ptr), extProb->DataSize * sizeof(float));

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
	d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
	d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);

	Copy2Dev((cl_mem)(d_a.ptr), extProb->h_a, extProb->DataSize * sizeof(float));
	Copy2Dev((cl_mem)(d_b.ptr), extProb->h_b, extProb->DataSize * sizeof(float));

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState ReducAddSolution::GetBackResult()
{
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(extProb->h_c, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void ReducAddSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                         */
/************************************************************************/
E_ReturnState ReducAddSolution::GenerateSolution()
{
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;
	T_ExtReducAddSolutionConfig * extSol = (T_ExtReducAddSolutionConfig *)SolutionConfig->extConfig;

	extSol->TileGroup = extProb->ReducSize / extSol->Tile;

	// ======================================================================
	// 生成worksize
	// ======================================================================
	if (extSol->Methord == 1)
	{
		SolutionConfig->l_wk0 = WAVE_SIZE;
		SolutionConfig->l_wk1 = 1;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = CU_NUM * SolutionConfig->l_wk0;
		SolutionConfig->g_wk1 = 1;
		SolutionConfig->g_wk2 = 1;
	}
	else if (extSol->Methord == 2)
	{
		SolutionConfig->l_wk0 = WAVE_SIZE;
		SolutionConfig->l_wk1 = extSol->TileGroup;
		SolutionConfig->l_wk2 = 1;
		SolutionConfig->g_wk0 = CU_NUM * SolutionConfig->l_wk0;
		SolutionConfig->g_wk1 = extSol->TileGroup;
		SolutionConfig->g_wk2 = 1;
	}

	// ======================================================================
	// 生成代码
	// ======================================================================
	SolutionConfig->KernelName = "ReducAdd";
	SolutionConfig->KernelFile = "ReducAdd.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterReducAdd * kw = new KernelWriterReducAdd(ProblemConfig, SolutionConfig);
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
E_ReturnState ReducAddProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtReducAddProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtReducAddProblemConfig();
	extProblemConfig->ReducSize = 10 * 5;
	extProblemConfig->VectorSize = 64;
	extProblemConfig->DataSize = extProblemConfig->ReducSize * extProblemConfig->VectorSize;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "(10x5)x64";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState ReducAddProblem::InitHost()
{
	std::cout << "Reduction Add init" << ProblemConfig->ConfigName << std::endl;
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;

	ProblemConfig->Calculation = extProb->DataSize; 
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

	extProb->h_a = (float*)HstMalloc(extProb->DataSize * sizeof(float));
	extProb->h_b = (float*)HstMalloc(extProb->DataSize * sizeof(float));
	extProb->h_c = (float*)HstMalloc(extProb->DataSize * sizeof(float));
	extProb->c_ref = (float*)HstMalloc(extProb->DataSize * sizeof(float));
	
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		for (int j = 0; j < extProb->ReducSize; j++)
		{
			extProb->h_a[i + j * extProb->VectorSize] = i + j;
		}
		extProb->h_c[i] = 0;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState ReducAddProblem::Host()
{
	printf("Reduction Add host.\n");
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < extProb->VectorSize; i++)
	{
		float tmp = 0;
		for (int j = 0; j < extProb->ReducSize; j++)
		{
			tmp += extProb->h_a[i + j * extProb->VectorSize];
		}
		extProb->c_ref[i] = tmp;
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState ReducAddProblem::Verify()
{
	printf("Reduction Add verify.\n");
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;
		
	float diff = 0;
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		diff += (extProb->c_ref[i] - extProb->h_c[i]) * (extProb->c_ref[i] - extProb->h_c[i]);
	}
	diff /= extProb->VectorSize;

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
void ReducAddProblem::ReleaseHost()
{
	printf("Reduction Add destroy.\n");
	T_ExtReducAddProblemConfig * extProb = (T_ExtReducAddProblemConfig *)ProblemConfig->extConfig;

	HstFree(extProb->h_a);
	HstFree(extProb->h_b);
	HstFree(extProb->h_c);
	HstFree(extProb->c_ref);
}
#pragma endregion
