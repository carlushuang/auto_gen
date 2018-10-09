#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterVectAdd :public krnelWriter::KernelWriter
{
public:
	KernelWriterVectAdd(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtVectAddProblemConfig * extProbCfg;	// ��ǰ���ڴ����������չ����
	T_ExtVectAddSolutionConfig * extSolCfg;	// ��ǰ���ڴ���Ľ��������չ����

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	krnelWriter::Var * v_a_addr;
	krnelWriter::Var * v_b_addr;
	krnelWriter::Var * v_c_addr;

	void writeProgram();
};
