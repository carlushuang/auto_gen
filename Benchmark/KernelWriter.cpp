
#include "KernelWriter.h"

namespace krnelWriter
{
	void KernelWriter::writeSignature()
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

	void KernelWriter::writeMetadata()
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
	void KernelWriter::initialDefaultGprs()
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
	void KernelWriter::writeCodeObj()
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
	
	/************************************************************************/
	/* 计算线性地址			                                                */
	/************************************************************************/
	void KernelWriter::f_linear_addr(Var * s_base_addr, Var * v_addr)
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
	
	/************************************************************************/
	/* 设置buffer描述符		                                                */
	/************************************************************************/
	E_ReturnState KernelWriter::f_set_buffer_desc(
		Var * s_desc,
		Var * s_base,
		uint stride,
		uint record_num,
		bool add_tid_en,
		e_desc_num_fmt num_fmt,
		e_desc_dat_fmt dat_fmt,
		bool swizzle_en,
		e_desc_idx_stride idx_stride,
		bool cache_swizzle)
	{
		// desc0
		op2("s_mov_b64", s_desc, s_base);

		// desc1
		uint dsc1_tmp = stride & 0x3FFF;
		dsc1_tmp = dsc1_tmp << 16;
		if (cache_swizzle == true)
		{
			dsc1_tmp |= (uint)1 << 30;
		}
		if (swizzle_en == true)
		{
			dsc1_tmp |= (uint)1 << 31;
		}
		op3("s_or_b32", *s_desc + 1, *s_desc + 1, dsc1_tmp);

		// desc2
		op2("s_mov_b32", *s_desc + 2, record_num);

		// desc3
		uint dsc3_tmp = ((uint)num_fmt & 0x7) << 12;
		if (add_tid_en == true)
		{
			dsc3_tmp |= ((stride & 0x3C000) >> 14) << 15;
		}
		else
		{
			dsc3_tmp |= ((uint)dat_fmt & 0xF) << 15;
		}
		if (swizzle_en == true)
		{
			dsc3_tmp |= (uint)idx_stride << 21;
		}
		if (add_tid_en == true)
		{
			dsc3_tmp |= 1 << 23;
		}
		op2("s_mov_b32", *s_desc + 3, dsc3_tmp);

		// error check
		std::string str = "";
		if (s_desc->type != E_VarType::VAR_SGPR)
		{
			str.append("buffer descriptor not sgpr");
			wrLine(str);
			return E_ReturnState::FAIL;
		}
		if (s_desc->sgpr.len != 4)
		{
			str.append("buffer descriptor not 4-dword");
			wrLine(str);
			return E_ReturnState::FAIL;
		}
		if (s_base->type != E_VarType::VAR_SGPR)
		{
			str.append("buffer obj base address not sgpr");
			wrLine(str);
			return E_ReturnState::FAIL;
		}

		return E_ReturnState::SUCCESS;
	}
}
