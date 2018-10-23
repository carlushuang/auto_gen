#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterIsaLds :public krnelWriter::KernelWriter
{
public:
	KernelWriterIsaLds(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtLdsProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtLdsSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	krnelWriter::Var * v_a_addr;
	krnelWriter::Var * v_b_addr;
	krnelWriter::Var * v_c_addr;

	void writeProgram();
};
