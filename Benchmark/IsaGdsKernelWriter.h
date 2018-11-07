#pragma once

#include "SampleConfig.h"
#include "KernelWriter.h"

namespace AutoGen
{
	class KernelWriterIsaGds :public KernelWriter
	{
	public:
		KernelWriterIsaGds(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg) :
			KernelWriter(probCfg, solCfg)
		{
			extProbCfg = (T_ExtGdsProblemConfig *)problemConfig->extConfig;
			extSolCfg = (T_ExtGdsSolutionConfig *)solutionConfig->extConfig;

			N = extProbCfg->VectorSize;
		}

	protected:
		int N = 0;
		T_ExtGdsProblemConfig * extProbCfg;	// 当前正在处理的问题扩展配置
		T_ExtGdsSolutionConfig * extSolCfg;	// 当前正在处理的解决方案扩展配置

		Var * s_ptr_a;
		Var * s_ptr_b;
		Var * s_ptr_c;

		Var * v_a_addr;
		Var * v_b_addr;
		Var * v_c_addr;
		
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
			wrLine("enable_ordered_append_gds = 0");
			wrLine("gds_segment_byte_size = 0");
			//wrLine("gds_segment_byte_size = " + d2s(64 * 4));
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

		void writeProgram()
		{
			s_ptr_a = newSgpr("s_ptr_a", 2, 2);
			s_ptr_b = newSgpr("s_ptr_b", 2, 2);
			s_ptr_c = newSgpr("s_ptr_c", 2, 2);

			v_a_addr = newVgpr("v_a_addr", 2, 2);
			v_b_addr = newVgpr("v_b_addr", 2, 2);
			v_c_addr = newVgpr("v_c_addr", 2, 2);

			Var * v_tmp0 = newVgpr("v_tmp0");
			Var * v_tmp1 = newVgpr("v_tmp1");

			s_load_dword(2, s_ptr_a, s_kernelArg, 0x00);
			s_load_dword(2, s_ptr_b, s_kernelArg, 0x08);
			s_load_dword(2, s_ptr_c, s_kernelArg, 0x10);
			s_wait_lgkmcnt(0);

			//	f_linear_addr(s_ptr_a, v_a_addr);
			//	f_linear_addr(s_ptr_b, v_b_addr);
			f_linear_addr(s_ptr_c, v_c_addr);


			/************************************************************************/
			/* ==================================================================== */
			/* ds_gws_init															*/
			/* ds_gws_sema_br														*/
			/* ds_gws_sema_p														*/
			/* ds_gws_sema_v														*/
			/* ds_gws_sema_release_all												*/
			/* ==================================================================== */
			/* 例:																	*/
			/************************************************************************/
			ldsByteCount += 4;	// 申请lds字节数
			Var * v_gds_addr = newVgpr("v_gds_addr");

			op3("v_lshlrev_b32", v_gds_addr, s_gid_x, 2);
			op2("v_mov_b32", v_gds_addr, 0);

			op2("s_cmp_eq_u32", s_gid_x, 0);
			op1("s_cbranch_scc0", l_end_prg);
			op2("s_mov_b64", "exec", 1);


			op2("v_mov_b32", v_tmp0, 0);
			ds_write_dword(1, v_gds_addr, v_tmp0, 0, true, true);
			s_wait_cnt0();

			ds_read_dword(1, v_tmp1, v_gds_addr, 0, true, true);
			s_wait_cnt0();


			//	op2("ds_gws_init", v_tmp0, "gds");


			op2("v_cvt_f32_u32", v_tmp1, v_tmp1);
			flat_store_dword(1, v_c_addr, v_tmp1, "off");

			delVar(v_gds_addr);

			/************************************************************************/
			/************************************************************************/

			delVar(v_tmp0);
			delVar(v_tmp1);
		}
	};
}
