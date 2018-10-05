#pragma once

#include "IsaDemoConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterIsaSmem :public krnelWriter::KernelWriter
{
public:
	KernelWriterIsaSmem(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtSmemProblemConfig * extProbCfg;	// ��ǰ���ڴ����������չ����
	T_ExtSmemSolutionConfig * extSolCfg;	// ��ǰ���ڴ���Ľ��������չ����

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	void writeProgram();
};
