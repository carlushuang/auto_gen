#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"
#include "ProblemControl.h"

class KernelWriterIsaGds :public krnelWriter::KernelWriter
{
public:
	KernelWriterIsaGds(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg);

protected:
	int N = 0;
	T_ExtGdsProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
	T_ExtGdsSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

	krnelWriter::Var * s_ptr_a;
	krnelWriter::Var * s_ptr_b;
	krnelWriter::Var * s_ptr_c;

	krnelWriter::Var * v_a_addr;
	krnelWriter::Var * v_b_addr;
	krnelWriter::Var * v_c_addr;

	void writeProgram();

	void writeCodeObj()
	{
		setTable(1);
		wrLine(".amd_kernel_code_t");
		indent();
		wrLine("enable_sgpr_private_segment_buffer = 1");
		wrLine("enable_sgpr_kernarg_segment_ptr = 1");
		wrLine("enable_sgpr_workgroup_id_x = 1");
		wrLine("enable_sgpr_workgroup_id_y = 1");
		wrLine("enable_sgpr_workgroup_id_z = 1");
		wrLine("enable_vgpr_workitem_id = 2");
		wrLine("is_ptr64 = 1");
		wrLine("float_mode = 240");
		wrLine("granulated_wavefront_sgpr_count = " + d2s((sgprCountMax - 1) / 4));
		wrLine("granulated_workitem_vgpr_count = " + d2s((vgprCountMax - 1) / 4));
		wrLine("user_sgpr_count = 6");
		wrLine("wavefront_sgpr_count = " + d2s(sgprCountMax));
		wrLine("workitem_vgpr_count = " + d2s(vgprCountMax));
		wrLine("kernarg_segment_byte_size = 56");
		wrLine("workgroup_group_segment_byte_size = " + d2s(ldsByteCount));
		wrLine("enable_ordered_append_gds = 1");
		wrLine("gds_segment_byte_size = " + d2s(64 * 4));
		backSpace();
		wrLine(".end_amd_kernel_code_t");		
		wrLine("");
	}
	
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
		wrLine("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 256, PrivateSegmentFixedSize : 256, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 512 }");
		wrLine("        Args:");
		wrLine("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
		wrLine("        - { Name: d_wei , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
		wrLine("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
		wrLine("      }");
		wrLine("}");
		wrLine(".end_amd_amdgpu_hsa_metadata");
		wrLine("");
	}
}; 
