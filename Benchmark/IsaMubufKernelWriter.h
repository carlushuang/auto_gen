#pragma once

#include "IsaDemoConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterIsaMubuf :public krnelWriter::KernelWriter
{
public:
	KernelWriterIsaMubuf(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtMubufProblemConfig * extProbCfg;	// ��ǰ���ڴ����������չ����
	T_ExtMubufSolutionConfig * extSolCfg;	// ��ǰ���ڴ���Ľ��������չ����

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	krnelWriter::Var * v_a_addr;
	krnelWriter::Var * v_b_addr;
	krnelWriter::Var * v_c_addr;

	void writeProgram();
};
