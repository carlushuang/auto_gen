/************************************************************************/
/* 这里定义的是依赖于问题配置的相关生成kernel的函数							*/
/* 比如group size，传入参数列表等等										*/
/* 因此只需要include ProblemControl.h									*/
/************************************************************************/
#pragma once

#include "KernelWriterBasic.h"
#include "ProblemControl.h"

#include <sys/stat.h>

namespace krnelWriter
{
	class KernelWriter : public KernelWriterBasic
	{
	public:
		KernelWriter(T_ProblemConfig * probCfg, T_SolutionConfig * solCfg)
			: KernelWriterBasic(E_IsaArch::Gfx900)
		{
			problemConfig = probCfg;
			solutionConfig = solCfg;

			kernelName = solutionConfig->KernelName;
			kernelFile = solutionConfig->KernelFile;
			groupSize0 = solutionConfig->l_wk0;
			groupSize1 = solutionConfig->l_wk1;
			groupSize2 = solutionConfig->l_wk2;
		}

	public:
		void GenKernelString()
		{
			clearString();
			writeContent();

			clearString();
			writeSignature();
			writeContent();
			writeMetadata();
		}
		void SaveKernelString2File()
		{
			std::string kernelPath = "../../../Kernels/";
			
			if (access(kernelPath.c_str(), F_OK) == -1)
			{
				::mkdir(kernelPath.c_str(), 0777);
			}

			std::string SrcFileName = kernelPath + kernelFile;
			std::ofstream fout(SrcFileName, std::ios::out);
			if (!fout.is_open())
			{
				FATAL("can't open save file");
			}
			fout.write(KernelString.c_str(), KernelString.length());
			fout.close();
		}
		void PrintKernelString()
		{
			printf("/************************************************************************************/\n");
			printf("/***** START  %s KERNEL *****/\n", kernelName.c_str());
			printf("/************************************************************************************/\n");
			printf(KernelString.c_str());
			printf("/************************************************************************************/\n");
			printf("/***** END  %s KERNEL *****/\n", kernelName.c_str());
			printf("/************************************************************************************/\n");
		}

	protected:
		T_ProblemConfig * problemConfig;
		T_SolutionConfig * solutionConfig;
		std::string kernelName;
		std::string kernelFile;
		int groupSize0, groupSize1, groupSize2;

		Var * s_privateSeg;
		Var * s_kernelArg;
		Var * s_gid_x;
		Var * s_gid_y;
		Var * s_gid_z;

		Var * v_tid_x;
		Var * v_tid_y;
		Var * v_tid_z;

		Var * START_PROG;
		Var * END_PROG;

		/************************************************************************/
		/* kernel文件生成函数                                                    */
		/************************************************************************/
		void writeSignature()
		{
			setTable(0);
			wrLine(".hsa_code_object_version 2, 1");
			wrLine(".hsa_code_object_isa 9, 0, 0, \"AMD\", \"AMDGPU\"");
			wrLine("");
			wrLine(".text");
			wrLine(".globl " + kernelName);
			wrLine(".p2align 8");
			wrLine(".type " + kernelName + ",@function");
			wrLine(".amdgpu_hsa_kernel " + kernelName);
			wrLine("");
		}
		void writeContent()
		{
			initialDefaultGprs();
			setTable(0);
			wrLine(kernelName + ":");
			writeCodeObj();
			_writeProgram();
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
			wrLine("        Attrs: { ReqdWorkGroupSize: [ " + d2s(groupSize0) + ", 1, 1 ] }");
			wrLine("        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize : 0, PrivateSegmentFixedSize : 0, KernargSegmentAlign : 8, WavefrontSize : 64, MaxFlatWorkGroupSize : 512 }");
			wrLine("        Args:");
			wrLine("        - { Name: d_in  , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
			wrLine("        - { Name: d_wei , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global, IsConst : true }");
			wrLine("        - { Name: d_out , Size : 8, Align : 8, ValueKind : GlobalBuffer, ValueType : F32, TypeName : 'float*', AddrSpaceQual : Global  }");
			wrLine("      }");
			wrLine("}");
			wrLine(".end_amd_amdgpu_hsa_metadata");
			wrLine("");
		}

		/************************************************************************/
		/* kernel 函数内容生成函数                                                */
		/************************************************************************/
		void initialDefaultGprs()
		{
			s_privateSeg = newSgpr("s_privateSeg", 4);
			s_kernelArg = newSgpr("s_kernelArg", 2);
			s_gid_x = newSgpr("s_gid_x");
			s_gid_y = newSgpr("s_gid_y");
			s_gid_z = newSgpr("s_gid_z");

			v_tid_x = newVgpr("v_tid_x");

			START_PROG = newLaber("START_PROG");
			END_PROG = newLaber("END_PROG");
		}
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
			wrLine("enable_vgpr_workitem_id = 0");
			wrLine("is_ptr64 = 1");
			wrLine("float_mode = 240");
			wrLine("granulated_wavefront_sgpr_count = " + d2s((sgprCountMax - 1) / 4));
			wrLine("granulated_workitem_vgpr_count = " + d2s((vgprCountMax - 1) / 4));
			wrLine("user_sgpr_count = 6");
			wrLine("wavefront_sgpr_count = " + d2s(sgprCountMax));
			wrLine("workitem_vgpr_count = " + d2s(vgprCountMax));
			wrLine("kernarg_segment_byte_size = 56");
			wrLine("workgroup_group_segment_byte_size = " + d2s(ldsByteCount));
			backSpace();
			wrLine(".end_amd_kernel_code_t");
			wrLine("");
		}
		void _writeProgram()
		{
			setTable(0);
			wrLine(getVar(START_PROG) + ":");
			indent();
			writeProgram();
			setTable(0);
			wrLine(getVar(END_PROG) + ":");
			indent();
			wrLine("s_endpgm\n");
			clrVar();
		}
		virtual void writeProgram() = 0;

		/************************************************************************/
		/* 常用kernel函数														 */
		/************************************************************************/
		void f_linear_addr(Var * s_base_addr, Var * v_addr)
		{
			Var * v_tmp1 = newVgpr("v_tmp1");
			Var * v_tmp2 = newVgpr("v_tmp2");

			op3("v_lshlrev_b32", v_tmp1, log2(groupSize0), s_gid_x);
			op4("v_add_lshl_u32", v_tmp1, v_tmp1, v_tid_x, 2);

			op2("v_mov_b32", v_tmp2, *s_base_addr + 1);
			op4("v_add_co_u32", v_addr, "vcc", s_base_addr, v_tmp1);
			op5("v_addc_co_u32", *v_addr + 1, "vcc", 0, v_tmp2, "vcc");

			delVar(v_tmp1);
			delVar(v_tmp2);
		}
	};
}