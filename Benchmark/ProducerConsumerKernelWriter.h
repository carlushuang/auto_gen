#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterProducerConsumer :public krnelWriter::KernelWriter
{
public:
	KernelWriterProducerConsumer(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtProducerConsumerProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtProducerConsumerSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_c;
	krnelWriter::Var * s_ptr_sig;

	krnelWriter::Var * s_a_addr;
	krnelWriter::Var * s_c_addr;
	krnelWriter::Var * s_cu_sig_addr;

	void writeProgram(); 
	
	void writeMetadata()
	{
		setTable(0);
		wrLine(".amd_amdgpu_hsa_metadata");
		wrLine("{ Version: [1, 0],");
		wrLine("  Kernels :");
		wrLine("    - { Name: " + kernelName + ",");
		wrLine("        SymbolName: " + kernelName + ",");
		wrLine("        Language: OpenCL C, LanguageVersion: [ 1, 2 ],");
		wrLine("        Attrs: { ReqdWorkGroupSize: [ " + d2s(groupSize0) + ", " + d2s(groupSize1) + ", " + d2s(groupSize2) + " ] }");
		wrLine("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 512 }");
		wrLine("        Args:");
		wrLine("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
		wrLine("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
		wrLine("        - { Name: d_sig , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
		wrLine("      }");
		wrLine("}");
		wrLine(".end_amd_amdgpu_hsa_metadata");
		wrLine("");
	}
};
