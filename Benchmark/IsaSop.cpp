
#include "IsaSop.h"

/************************************************************************/
/* solution����                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* ����problem������solution�����ռ�                                      */
/************************************************************************/
E_ReturnState SopSolution :: GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtSopSolutionConfig * extSol;

	extSol = new T_ExtSopSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* �����Դ�                                                            */
/************************************************************************/
E_ReturnState SopSolution::InitDev()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;

	DevMalloc((void**)&(d_a.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_b.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_c.ptr), extProb->VectorSize * sizeof(float));

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
	d_b.size = sizeof(cl_mem);	d_b.isVal = false;	SolutionConfig->KernelArgus->push_back(d_b);
	d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);

	Copy2Dev((cl_mem)(d_a.ptr), extProb->h_a, extProb->VectorSize * sizeof(float));
	Copy2Dev((cl_mem)(d_b.ptr), extProb->h_b, extProb->VectorSize * sizeof(float));

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* ���ؽ��                                                            */
/************************************************************************/
E_ReturnState SopSolution::GetBackResult()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(extProb->h_c, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
}

/************************************************************************/
/* �ͷ��Դ�	                                                           */
/************************************************************************/
void SopSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* ����solution��������source, complier��worksize                         */
/************************************************************************/
E_ReturnState SopSolution::GenerateSolution()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;
	T_ExtSopSolutionConfig * extSol = (T_ExtSopSolutionConfig *)SolutionConfig->extConfig;

	// ======================================================================
	// ����worksize
	// ======================================================================
	SolutionConfig->l_wk0 = WAVE_SIZE;
	SolutionConfig->l_wk1 = 1;
	SolutionConfig->l_wk2 = 1;
	SolutionConfig->g_wk0 = extProb->VectorSize;
	SolutionConfig->g_wk1 = 1;
	SolutionConfig->g_wk2 = 1;
		
	// ======================================================================
	// ���ɴ���
	// ======================================================================
	SolutionConfig->KernelName = "IsaSop";
	SolutionConfig->KernelFile = "IsaSopAutoGen.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterIsaSop * kw = new KernelWriterIsaSop(ProblemConfig, SolutionConfig);
	kw->GenKernelString();
	kw->PrintKernelString();
	kw->SaveKernelString2File();

	return E_ReturnState::SUCCESS;
}
#pragma endregion

/************************************************************************/
/* �������                                                             */
/************************************************************************/
#pragma region ProblemRegion
/************************************************************************/
/* ��������ռ�													        */
/************************************************************************/
E_ReturnState SopProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * probCfg;
	T_ExtSopProblemConfig * extProb;

	probCfg = new T_ProblemConfig();

	extProb = new T_ExtSopProblemConfig();
	extProb->VectorSize = 1024;
	probCfg->extConfig = extProb;

	ProblemConfigList->push_back(probCfg);
}

/************************************************************************/
/* ������ʼ��                                                            */
/************************************************************************/
E_ReturnState SopProblem::InitHost()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;
		
	extProb->h_a = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_b = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_c = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->c_ref = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	ProblemConfig->Calculation = extProb->VectorSize;
		
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->h_a[i] = i;
		extProb->h_b[i] = 2;
		extProb->h_c[i] = 0;
	}

	return E_ReturnState::SUCCESS; 
} 

/************************************************************************/
/* HOST��                                                               */
/************************************************************************/
E_ReturnState SopProblem::Host()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->c_ref[i] = extProb->h_a[i] + extProb->h_b[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* У��                                                                 */
/************************************************************************/
E_ReturnState SopProblem::Verify()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;
		
	float diff = 0;
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		diff += (extProb->c_ref[i] - extProb->h_c[i]) * (extProb->c_ref[i] - extProb->h_c[i]);
	}
	diff /= extProb->VectorSize;
		
	printf("mean err = %.1f.\n", diff);
	if (!(diff >= 0 && diff < MIN_FP32_ERR))
	{
		printf("err = %.2f\n", diff);
		INFO("verify failed!");
		return E_ReturnState::FAIL;
	}
	INFO("verify success.");
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* �ͷ�                                                                  */
/************************************************************************/
void SopProblem::ReleaseHost()
{
	T_ExtSopProblemConfig * extProb = (T_ExtSopProblemConfig *)ProblemConfig->extConfig;

	HstFree(extProb->h_a);
	HstFree(extProb->h_b);
	HstFree(extProb->h_c);
	HstFree(extProb->c_ref);
}
#pragma endregion
 