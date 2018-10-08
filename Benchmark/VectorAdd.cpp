
#include "VectorAdd.h"

/************************************************************************/
/* solution����                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* ����problem������solution�����ռ�                                      */
/************************************************************************/
E_ReturnState VectAddSolution::GenerateSolutionConfigs()
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
E_ReturnState VectAddSolution::InitDev()
{
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

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
/* ���ؽ��                                                            */
/************************************************************************/
E_ReturnState VectAddSolution::GetBackResult()
{
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(exCfg->h_c, (cl_mem)(d_c.ptr), exCfg->VectorSize * sizeof(float));
}

/************************************************************************/
/* �ͷ��Դ�	                                                           */
/************************************************************************/
void VectAddSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_b.ptr));
	DevFree((cl_mem)(d_c.ptr));
}

/************************************************************************/
/* ����solution��������source, complier��worksize                         */
/************************************************************************/
E_ReturnState VectAddSolution::GenerateSolution()
{
	T_ExtVectAddProblemConfig * extProb = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;
	T_ExtVectAddSolutionConfig * extSol = (T_ExtVectAddSolutionConfig *)SolutionConfig->extConfig;

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
/* �������                                                             */
/************************************************************************/
#pragma region ProblemRegion
/************************************************************************/
/* ��������ռ�													        */
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
/* ������ʼ��                                                            */
/************************************************************************/
E_ReturnState VectAddProblem::InitHost()
{
	std::cout << "Vector Add init" << ProblemConfig->ConfigName << std::endl;
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

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
/* HOST��                                                               */
/************************************************************************/
E_ReturnState VectAddProblem::Host()
{
	printf("Vector Add host.\n");
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < exCfg->VectorSize; i++)
	{
		exCfg->c_ref[i] = exCfg->h_a[i] + exCfg->h_b[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* У��                                                                 */
/************************************************************************/
E_ReturnState VectAddProblem::Verify()
{
	printf("Vector Add verify.\n");
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;
		
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
/* �ͷ�                                                                  */
/************************************************************************/
void VectAddProblem::ReleaseHost()
{
	printf("Vector Add destroy.\n");
	T_ExtVectAddProblemConfig * exCfg = (T_ExtVectAddProblemConfig *)ProblemConfig->extConfig;

	HstFree(exCfg->h_a);
	HstFree(exCfg->h_b);
	HstFree(exCfg->h_c);
	HstFree(exCfg->c_ref);
}
#pragma endregion
