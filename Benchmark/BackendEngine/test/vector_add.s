.hsa_code_object_version 2,0
.hsa_code_object_isa 9, 0, 0, "AMD", "AMDGPU"

.text
.p2align 8
.amdgpu_hsa_kernel vector_add

// CAUTION! this asm file need further debug

vector_add:
    .amd_kernel_code_t	
        // enable_sgpr_private_segment_buffer  = 1     //(use 4 SGPR)
        enable_sgpr_dispatch_ptr            = 1     //(use 2 SGPR)
        enable_sgpr_kernarg_segment_ptr     = 1     //(use 2 SGPR) 64 bit address of Kernarg segment.
        enable_sgpr_workgroup_id_x          = 1     //(use 1 SGPR) 32 bit work group id in X dimension of grid for wavefront. Always present.
        // ! user_sgpr is before enable_sgpr_workgroup_id_x(called system sgpr, alloced by SPI)
        // SGPRs before the Work-Group Ids are set by CP using the 16 User Data registers.
        user_sgpr_count                     = 4     // total number of above enabled sgpr. does not contains enable_sgpr_* start from tgid_x

        enable_vgpr_workitem_id             = 0     // only vgpr0 used for workitem id x

        is_ptr64                            = 1     // ?
        float_mode                          = 192   // 0xc0

        wavefront_sgpr_count                = 16
        workitem_vgpr_count                 = 10
        granulated_workitem_vgpr_count      = 2     // max(0, ceil(vgprs_used / 4) - 1)
        granulated_wavefront_sgpr_count     = 1     // max(0, ceil(sgprs_used / 8) - 1)

        kernarg_segment_byte_size           = 28    // kernarg segment size(byte)
        // workgroup_group_segment_byte_size   = 0	    //[caculated] group segment memory required by a work-group in bytes		
    .end_amd_kernel_code_t

    // init state: s[0:1] hsa_kernel_dispatch_packet_t, s[2:3] kernarg segment, s[4] workgroup id
    s_load_dword        s5, s[0:1], 0x4             // load hsa_kernel_dispatch_packet_t.workgroup_size_x, uint16_t
    s_load_dword        s6, s[0:1], 0xc             // load hsa_kernel_dispatch_packet_t.grid_size_x, uint32_t
    s_load_dword        s7, s[2:3], 0x18            // kernarg, number, s7
    s_waitcnt           lgkmcnt(0)

    s_and_b32           s5, s5, 0xffff              // 16 bit
    s_mul_i32           s4, s4, s5                  // group-1 * 
    // v0: blockIdx.x * blockDim.x + threadIdx.x
    v_add_u32_e32       v0, s4, v0                  // global work item id
    v_cmp_gt_i32_e32    vcc, s7, v0
    s_mov_b64           exec, vcc
    s_cbranch_execz     Lend

    s_load_dwordx2      s[0:1], s[2:3], 0x0         // kernarg, vec_a
    s_load_dwordx2      s[8:9], s[2:3], 0x8         // kernarg, vec_b
    s_load_dwordx2      s[2:3], s[2:3], 0x10        // kernarg, vec_c
    s_waitcnt           lgkmcnt(0)

Lbegin:

    v_mov_b32_e32       v1, 0                       // higher 32 bit
    v_lshlrev_b64       v[2:3], 2, v[0:1]           // workitem-> byte offset

    // v[4:5] a
    v_mov_b32_e32       v1, s1
    v_add_co_u32_e32    v4, vcc, s0, v2             // kernarg a addr lo32
    v_addc_co_u32_e32   v5, vcc, v1, v3, vcc        // kernarg a addr hi32

    // v[7:8] b
    v_mov_b32_e32       v6, s9
    v_add_co_u32_e32    v7, vcc, s8, v2             // kernarg b addr lo32
    v_addc_co_u32_e32   v8, vcc, v6, v3, vcc        // kernarg b addr hi32

    // v[2:3] c
    v_mov_b32_e32       v6, s3
    v_add_co_u32_e32    v2, vcc, s2, v2             // kernarg c addr lo32
    v_addc_co_u32_e32   v3, vcc, v6, v3, vcc        // kernarg c addr hi32

    flat_load_dword     v4, v[4:5]                  // load kernarg a
    # s_nop 0
    flat_load_dword     v5, v[7:8]                  // load kernarg b
    s_waitcnt           vmcnt(0)
    v_add_f32_e32       v5, v4, v5                  // a+=b
    flat_store_dword    v[2:3], v5                  // store
    s_waitcnt           vmcnt(0)

    v_add_u32_e32       v0, s6, v0                  // i += blockDim.x*gridDim.x
    v_cmp_gt_i32_e32    vcc, s7, v0
    s_mov_b64           exec, vcc
    s_cbranch_execnz    Lbegin

Lend:
    s_endpgm

.amd_amdgpu_hsa_metadata
{ Version: [ 1, 0 ],
  Kernels: 
    - { Name: vector_add, SymbolName: 'vector_add', Language: OpenCL C, LanguageVersion: [ 1, 2 ],
        Attrs: { ReqdWorkGroupSize: [ 64, 1, 1 ] }
        CodeProps: { KernargSegmentSize: 28, GroupSegmentFixedSize: 0, PrivateSegmentFixedSize: 0, KernargSegmentAlign: 8, WavefrontSize: 64, MaxFlatWorkGroupSize: 512 }
        Args:
        - { Name: d_a , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_b , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global, IsConst: true }
        - { Name: d_c , Size: 8, Align: 8, ValueKind: GlobalBuffer, ValueType: F32, TypeName: 'float*', AddrSpaceQual: Global }
        - { Name: cnt , Size: 4, Align: 4, ValueKind: ByValue, ValueType: I32, TypeName: 'int' }
      }
}
.end_amd_amdgpu_hsa_metadata
