#pragma once

#include "IsaDemoConfig.h"
#include "ProblemControl.h"
#include "IsaSmemKernelWriter.h"

/************************************************************************/
/* solution����                                                          */
/************************************************************************/
class SmemSolution : public SolutionCtrlBase
{
private:
	T_KernelArgu d_a, d_b, d_c;

public:
	/************************************************************************/
	/* ����problem������solution�����ռ�                                      */
	/************************************************************************/
	E_ReturnState GenerateSolutionConfigs();

	/************************************************************************/
	/* �����Դ�                                                            */
	/************************************************************************/
	E_ReturnState InitDev();

	/************************************************************************/
	/* ���ؽ��                                                            */
	/************************************************************************/
	E_ReturnState GetBackResult();

	/************************************************************************/
	/* �ͷ��Դ�	                                                           */
	/************************************************************************/
	void ReleaseDev();
	
	/************************************************************************/
	/* ����solution��������source, complier��worksize                         */
	/************************************************************************/
	E_ReturnState GenerateSolution();
};

/************************************************************************/
/* �������                                                             */
/************************************************************************/
class SmemProblem : public ProblemCtrlBase
{
public:
	SmemProblem(std::string name):ProblemCtrlBase(name)
	{
		Solution = new SmemSolution();
	}
	
	/************************************************************************/
	/* ��������ռ�													        */
	/************************************************************************/
	E_ReturnState GenerateProblemConfigs();

	/************************************************************************/
	/* ������ʼ��                                                            */
	/************************************************************************/
	E_ReturnState InitHost();

	/************************************************************************/
	/* HOST��                                                               */
	/************************************************************************/
	E_ReturnState Host();

	/************************************************************************/
	/* У��                                                                 */
	/************************************************************************/
	E_ReturnState Verify();

	/************************************************************************/
	/* �ͷ�                                                                  */
	/************************************************************************/
	void ReleaseHost();
};
 