/************************************************************************************/
// flat_load_dword		vgpr_dst, vgpr_addr                      flat_offset12 glc slc
//
// flat_offset12:
//		Specifies an immediate unsigned 12-bit offset, in bytes.
//		offset:{0..4095}
// glc:
//		If true, operation is globally coherent. Default 0.
// slc:
//		If true, operation is system coherent. Default 0.
/************************************************************************************/
.hsa_code_object_version 2,1
.hsa_code_object_isa 9,0,0,"AMD","AMDGPU"

.text
.globl IsaDs
.p2align 8
.type IsaDs,@function
.amdgpu_hsa_kernel IsaDs

/************************************************************************************/
/* 预定义																			*/
/************************************************************************************/
// ==================================================================================
// 常数定义
// ==================================================================================
.set LOCAL_SIZE_LOG2,	6

// ==================================================================================
// 输入参数排布
// ==================================================================================
a_ptr_off = 0x00
b_ptr_off = 0x08
c_ptr_off = 0x10

// ==================================================================================
// SGPR 排布
// ==================================================================================
s_arg = 4
s_gidx = 6

s_temp0 = 7
s_a_ptr = 8
s_b_ptr = 10
s_c_ptr = 12
s_temp1 = 14
s_temp2 = 16
s_temp3 = 18

// ==================================================================================
// VGPR 排布
// ==================================================================================
v_tid0 = 0
v_a_addr = 2
v_b_addr = 4
v_c_addr = 6
v_temp1 = 8
v_temp2 = 10
v_ds_off = 12


/************************************************************************************/
/* 主程序																			*/
/************************************************************************************/
IsaDs:
	// ===============================================================================
	// ===============================================================================
	.amd_kernel_code_t
		enable_sgpr_private_segment_buffer = 1
		enable_sgpr_kernarg_segment_ptr = 1
		enable_sgpr_workgroup_id_x = 1
		enable_sgpr_workgroup_id_y = 0
		enable_sgpr_workgroup_id_z = 0
		
		enable_vgpr_workitem_id = 0
	    
		is_ptr64 = 1
		float_mode = 240
		
		granulated_workitem_vgpr_count = 15
		granulated_wavefront_sgpr_count = 7
		user_sgpr_count = 6
		kernarg_segment_byte_size = 56
		wavefront_sgpr_count = 64
		workitem_vgpr_count = 64
		workgroup_group_segment_byte_size = 4		
	.end_amd_kernel_code_t
	
	// ===============================================================================
	// ===============================================================================
// Disassembly:        
	s_load_dwordx2 s[s_a_ptr:s_a_ptr+1], s[s_arg:s_arg+1], 0x0+a_ptr_off   
	s_load_dwordx2 s[s_b_ptr:s_b_ptr+1], s[s_arg:s_arg+1], 0x0+b_ptr_off
	s_load_dwordx2 s[s_c_ptr:s_c_ptr+1], s[s_arg:s_arg+1], 0x0+c_ptr_off
	s_waitcnt lgkmcnt(0)
	
	// -------------------------------------------------------------------------------
	// 计算输入下标: 
	// a_addr = a_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_a_ptr+1]
	v_add_co_u32 		v[v_a_addr], vcc, s[s_a_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_a_addr+1], vcc, 0, v[v_temp2], vcc
	// -------------------------------------------------------------------------------
	// 计算输入下标: 
	// b_addr = b_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_b_ptr+1]
	v_add_co_u32 		v[v_b_addr], vcc, s[s_b_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_b_addr+1], vcc, 0, v[v_temp2], vcc
		
	// -------------------------------------------------------------------------------
	// 计算输出下标: 
	// c_addr = c_ptr + (gid_x * local_size) + tid_x
	// -------------------------------------------------------------------------------
	v_lshlrev_b32 		v[v_temp1], 0x0 + LOCAL_SIZE_LOG2, s[s_gidx]
	v_add_lshl_u32 		v[v_temp1], v[v_temp1], v[v_tid0], 2
		
	v_mov_b32			v[v_temp2], s[s_c_ptr+1]
	v_add_co_u32 		v[v_c_addr], vcc, s[s_c_ptr], v[v_temp1]
	v_addc_co_u32 		v[v_c_addr+1], vcc, 0, v[v_temp2], vcc
  
	// -------------------------------------------------------------------------------
	// 计算
	// -------------------------------------------------------------------------------
	//global_load_dword                           v[v_temp1], v[v_a_addr:v_a_addr+1], off
    //s_waitcnt                                   vmcnt(0)
	//
    //v_lshlrev_b32                               v[v_ds_off], 2, v[v_tid0]
    //s_mov_b32                                   m0, -1
	//
	//ds_write_b32                                v[v_ds_off], v[v_temp1]
    //s_waitcnt                                   lgkmcnt(0)
    //ds_read_b32                                 v[v_temp2], v[v_ds_off]
    //s_waitcnt                                   lgkmcnt(0)
    //global_store_dword                          v[v_c_addr:v_c_addr+1], v[v_temp2], off
	
	//global_load_dword                           v[v_temp1], v[v_b_addr:v_b_addr+1], off
    //s_waitcnt                                   vmcnt(0)
	//
    //v_lshlrev_b32                               v[v_ds_off], 2, v[v_tid0]
    //s_mov_b32                                   m0, 2<<15
	//
	//v_add_f32									v[v_temp2], lds_direct, v[v_temp1]	
    //global_store_dword                          v[v_c_addr:v_c_addr+1], v[v_temp2], off


	s_endpgm                              
	
/************************************************************************************/
/* metadata																			*/
/************************************************************************************/	 
.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: IsaDs, SymbolName: 'IsaDs', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 24, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: a, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: b, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: c, Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global  }
      }
}
.end_amd_amdgpu_hsa_metadata
