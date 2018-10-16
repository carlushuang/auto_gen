
#include "ProducerConsumer.h"

/************************************************************************/
/* solution控制                                                          */
/************************************************************************/
#pragma region SolutionRegion
/************************************************************************/
/* 根据problem参数成solution参数空间                                      */
/************************************************************************/
E_ReturnState ProducerConsumerSolution::GenerateSolutionConfigs()
{
	T_SolutionConfig * solutionConfig;
	T_ExtProducerConsumerSolutionConfig * extSol;

	extSol = new T_ExtProducerConsumerSolutionConfig();
	solutionConfig = new T_SolutionConfig("AutoGen");
	solutionConfig->extConfig = extSol;
	SolutionConfigList->push_back(solutionConfig);

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 申请显存                                                            */
/************************************************************************/
E_ReturnState ProducerConsumerSolution::InitDev()
{
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;

	DevMalloc((void**)&(d_a.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_c.ptr), extProb->VectorSize * sizeof(float));
	DevMalloc((void**)&(d_sig.ptr), extProb->SignalSize * sizeof(float));

	SolutionConfig->KernelArgus = new std::list<T_KernelArgu>;
	d_a.size = sizeof(cl_mem);	d_a.isVal = false;	SolutionConfig->KernelArgus->push_back(d_a);
	d_c.size = sizeof(cl_mem);	d_c.isVal = false;	SolutionConfig->KernelArgus->push_back(d_c);
	d_sig.size = sizeof(cl_mem); d_sig.isVal = false;	SolutionConfig->KernelArgus->push_back(d_sig);

	Copy2Dev((cl_mem)(d_a.ptr), extProb->h_a, extProb->VectorSize * sizeof(float));

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 返回结果                                                            */
/************************************************************************/
E_ReturnState ProducerConsumerSolution::GetBackResult()
{
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;
	Copy2Hst(extProb->h_c, (cl_mem)(d_c.ptr), extProb->VectorSize * sizeof(float));
	Copy2Hst(extProb->h_sig, (cl_mem)(d_sig.ptr), extProb->SignalSize * sizeof(float));
}

/************************************************************************/
/* 释放显存	                                                           */
/************************************************************************/
void ProducerConsumerSolution::ReleaseDev()
{
	DevFree((cl_mem)(d_a.ptr));
	DevFree((cl_mem)(d_c.ptr));
	DevFree((cl_mem)(d_sig.ptr));
}

/************************************************************************/
/* 根据solution参数生成source, complier和worksize                         */
/************************************************************************/
E_ReturnState ProducerConsumerSolution::GenerateSolution()
{
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;
	T_ExtProducerConsumerSolutionConfig * extSol = (T_ExtProducerConsumerSolutionConfig *)SolutionConfig->extConfig;

	// ======================================================================
	// 生成worksize
	// ======================================================================
	SolutionConfig->l_wk0 = WAVE_SIZE;
	SolutionConfig->l_wk1 = 1;
	SolutionConfig->l_wk2 = 1;
	int group_num = extProb->VectorSize / SolutionConfig->l_wk0;
	SolutionConfig->g_wk0 = (group_num + 1) * 64 * SolutionConfig->l_wk0;
	SolutionConfig->g_wk1 = 1;
	SolutionConfig->g_wk2 = 1;

	// ======================================================================
	// 生成代码
	// ======================================================================
	SolutionConfig->KernelName = "ProducerConsumer";
	SolutionConfig->KernelFile = "ProducerConsumer.s";
	SolutionConfig->KernelSrcType = E_KernleType::KERNEL_TYPE_GAS_FILE;

	KernelWriterProducerConsumer * kw = new KernelWriterProducerConsumer(ProblemConfig, SolutionConfig);
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
E_ReturnState ProducerConsumerProblem::GenerateProblemConfigs()
{
	T_ProblemConfig * problemConfig;
	T_ExtProducerConsumerProblemConfig * extProblemConfig;

	// ----------------------------------------------------------------------
	// problem config 1
	extProblemConfig = new T_ExtProducerConsumerProblemConfig();
	extProblemConfig->VectorSize = 1024;
	extProblemConfig->SignalPerCU = 64;
	extProblemConfig->SignalSize = extProblemConfig->SignalPerCU * CU_NUM;

	problemConfig = new T_ProblemConfig();
	problemConfig->ConfigName = "1024";
	problemConfig->extConfig = extProblemConfig;

	ProblemConfigList->push_back(problemConfig);
}

/************************************************************************/
/* 参数初始化                                                            */
/************************************************************************/
E_ReturnState ProducerConsumerProblem::InitHost()
{
	std::cout << "Vector Add init" << ProblemConfig->ConfigName << std::endl;
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;

	ProblemConfig->Calculation = extProb->VectorSize; 
	ProblemConfig->TheoryElapsedTime = ProblemConfig->Calculation / RuntimeCtrlBase::DeviceInfo.Fp32Flops;
	printf("Calculation = %.3f G\n", ProblemConfig->Calculation * 1e-9);
	printf("TheoryElapsedTime = %.3f us \n", ProblemConfig->TheoryElapsedTime * 1e6);

	extProb->h_a = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_b = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_c = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
	extProb->h_sig = (float*)HstMalloc(extProb->SignalSize * sizeof(float));
	extProb->c_ref = (float*)HstMalloc(extProb->VectorSize * sizeof(float));
		
	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->h_a[i] = i;
		extProb->h_b[i] = 2;
		extProb->h_c[i] = 0;
	}
	for (int i = 0; i < extProb->SignalSize; i++)
	{
		extProb->h_sig[i] = 0;
	}

	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* HOST端                                                               */
/************************************************************************/
E_ReturnState ProducerConsumerProblem::Host()
{
	printf("Vector Add host.\n");
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;

	for (int i = 0; i < extProb->VectorSize; i++)
	{
		extProb->c_ref[i] = extProb->h_a[i] + extProb->h_b[i];
	}
	return E_ReturnState::SUCCESS;
}

/************************************************************************/
/* 校验                                                                 */
/************************************************************************/
E_ReturnState ProducerConsumerProblem::Verify()
{
	printf("Vector Add verify.\n");
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;
		
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
void ProducerConsumerProblem::ReleaseHost()
{
	printf("Vector Add destroy.\n");
	T_ExtProducerConsumerProblemConfig * extProb = (T_ExtProducerConsumerProblemConfig *)ProblemConfig->extConfig;

	HstFree(extProb->h_a);
	HstFree(extProb->h_b);
	HstFree(extProb->h_c);
	HstFree(extProb->h_sig);
	HstFree(extProb->c_ref);
}
#pragma endregion
