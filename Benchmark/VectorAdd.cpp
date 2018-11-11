
#include "VectorAdd.h"

using namespace AutoGen;
using namespace AutoTune;

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState VectAddSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtSolutionConfig * extSol;

	extSol = new T_ExtSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState VectAddSolution::InitDev()
{
	T_ExtProblemConfig * extProb = (T_ExtProblemConfig *)ProblemConfig->extConfig;
	T_ExtSolutionConfig * extSol = (T_ExtSolutionConfig *)SolutionConfig->extConfig;

	DevMalloc((void**)&(d_a.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_b.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_c.ptr), extProb->VectorSize * sizeof(float));

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	extSol->KernelArgus = SolutionConfig->KernelArgus;
	d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
	d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
	d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);
	//d_dbg_thd.size = sizeof(float);
	//d_dbg_thd.isVal = false;
	//SolutionConfig->KernelArgus->push_back(d_dbg_thd);
	//d_dbg_wave.size = sizeof(float);
	//d_dbg_wave.isVal = false;
	//SolutionConfig->KernelArgus->push_back(d_dbg_wave);

	Copy2Dev((cl_mem)(d_a.ptr), extProb->h_a, extProb->VectorSize * sizeof(float));
	Copy2Dev((cl_mem)(d_b.ptr), extProb->h_b, extProb->VectorSize * sizeof(float));

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState VectAddSolution::GetBackResult()
{
	T_ExtProblemConfig * extProb = (T_ExtProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(extProb->h_c, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void VectAddSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                         */
/************************************************************************/
E_ReturnState VectAddSolution::GenerateSolution()
{
	T_ExtProblemConfig * extProb = (T_ExtProblemConfig *)ProblemConfig->extConfig;
	T_ExtSolutionConfig * extSol = (T_ExtSolutionConfig *)SolutionConfig->extConfig;

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
	SolutionConfig->KernelName = "VectAdd";
	SolutionConfig->KernelFile = "VectAdd.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterVectAdd * kw = new KernelWriterVectAdd(ProblemConfig, SolutionConfig);
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
E_ReturnState VectAddProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtVectAddProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtVectAddProblemConfig();
	extProblemConfig->VectorSize = 1024;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "512";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState VectAddProblem::InitHost()
{
	std::cout << "Vector Add init" << ProblemConfig->ConfigName << std::endl;
	T_ExtVectAddProblemConfig * extProb = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

	ProblemConfig->Calculation = extProb->VectorSize; 
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

	extProb->h_a = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_b = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_c = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->c_ref = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->h_a[i] = i;
		extProb->h_b[i] = 2;
		extProb->h_c[i] = 0;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState VectAddProblem::Host()
{
	printf("Vector Add host.\n");
	T_ExtVectAddProblemConfig * extProb = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->c_ref[i] = extProb->h_a[i] + extProb->h_b[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState VectAddProblem::Verify()
{
	printf("Vector Add verify.\n");
	T_ExtVectAddProblemConfig * extProb = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;
		
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
void VectAddProblem::ReleaseHost()
{
	printf("Vector Add destroy.\n");
	T_ExtVectAddProblemConfig * extProb = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

	HstFree(extProb->h_a);
	HstFree(extProb->h_b);
	HstFree(extProb->h_c);
	HstFree(extProb->c_ref);
}
#pragma endregion
