
#include "IsaFlat.h"

/************************************************************************/
/* solution����                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* ����problem������solution�����ռ�                                      */
/************************************************************************/
E_ReturnState FlatSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtFlatSolutionConfig * extSol;

	extSol = new T_ExtFlatSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* �����Դ�                                                            */
/************************************************************************/
E_ReturnState FlatSolution::InitDev()
{
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;

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
E_ReturnState FlatSolution::GetBackResult()
{
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(extProb->h_c, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
}

/************************************************************************/
/* �ͷ��Դ�	                                                           */
/************************************************************************/
void FlatSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* ����solution��������source, complier��worksize                         */
/************************************************************************/
E_ReturnState FlatSolution::GenerateSolution()
{
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;
	T_ExtFlatSolutionConfig * extSol = (T_ExtFlatSolutionConfig *)SolutionConfig->extConfig;

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
	SolutionConfig->KernelName = "IsaFlat";
	SolutionConfig->KernelFile = "IsaFlatAutoGen.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterIsaFlat * kw = new KernelWriterIsaFlat(ProblemConfig, SolutionConfig);
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
E_ReturnState FlatProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtFlatProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtFlatProblemConfig();
	extProblemConfig->VectorSize = 1024;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "512";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* ������ʼ��                                                            */
/************************************************************************/
E_ReturnState FlatProblem::InitHost()
{
	std::cout << "Flat Instruction init" << ProblemConfig->ConfigName << std::endl;
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;

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
/* HOST��                                                               */
/************************************************************************/
E_ReturnState FlatProblem::Host()
{
	printf("flat instruction host.\n");
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->c_ref[i] = extProb->h_a[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* У��                                                                 */
/************************************************************************/
E_ReturnState FlatProblem::Verify()
{
	printf("flat instruction verify.\n");
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;
		
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
/* �ͷ�                                                                  */
/************************************************************************/
void FlatProblem::ReleaseHost()
{
	printf("flat instruction destroy.\n");
	T_ExtFlatProblemConfig * extProb = (T_ExtFlatProblemConfig *)ProblemConfig->extConfig;

	HstFree(extProb->h_a);
	HstFree(extProb->h_b);
	HstFree(extProb->h_c);
	HstFree(extProb->c_ref);
}
#pragma endregion
