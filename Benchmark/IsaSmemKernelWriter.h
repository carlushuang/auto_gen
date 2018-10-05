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
	T_ExtSmemProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtSmemSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	void writeProgram();
};
