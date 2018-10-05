#pragma once

#include "KernelWriterBasic.h"
#include "ProblemControl.h"

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
			std::string SrcFileName = "../../../Kernels/" + kernelFile;
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
		void writeSignature();
		void writeContent()
		{
			initialDefaultGprs();
			setTable(0);
			wrLine(kernelName + ":");
			writeCodeObj();
			_writeProgram();
		}
		void writeMetadata();

		/************************************************************************/
		/* kernel 函数内容生成函数                                                */
		/************************************************************************/
		void initialDefaultGprs();
		void writeCodeObj();
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
		void f_linear_addr(Var * s_base_addr, Var * v_addr);

		typedef enum desc_num_fmt_enum
		{
			num_fmt_unorm = 0,
			num_fmt_snorm = 1,
			num_fmt_uscaled = 2,
			num_fmt_sscaled = 3,
			num_fmt_uint = 4,
			num_fmt_sint = 5,
			num_fmt_reserved = 6,
			num_fmt_float = 7
		}e_desc_num_fmt;
		typedef enum desc_dat_fmt_enum
		{
			dat_fmt_invalid = 0,
			dat_fmt_8 = 1,
			dat_fmt_16 = 2,
			dat_fmt_8_8 = 3,
			dat_fmt_32 = 4,
			dat_fmt_16_16 = 5,
			dat_fmt_10_11_11 = 6,
			dat_fmt_11_11_10 = 7,
			dat_fmt_10_10_10_2 = 8,
			dat_fmt_2_10_10_10 = 9,
			dat_fmt_8_8_8_8 = 10,
			dat_fmt_32_32 = 11,
			dat_fmt_16_16_16_16 = 12,
			dat_fmt_32_32_32 = 13,
			dat_fmt_32_32_32_32 = 14,
			dat_fmt_reserved = 15
		}e_desc_dat_fmt;
		typedef enum desc_idx_stride_enum
		{
			idx_stride_8 = 0,
			idx_stride_16 = 1,
			idx_stride_32 = 2,
			idx_stride_64 = 3
		}e_desc_idx_stride;
		E_ReturnState f_set_buffer_desc(
			Var * s_desc,
			Var * s_base,
			uint stride,
			uint record_num,
			bool add_tid_en,
			e_desc_num_fmt num_fmt = e_desc_num_fmt::num_fmt_float,
			e_desc_dat_fmt dat_fmt = e_desc_dat_fmt::dat_fmt_32,
			bool swizzle_en = false,
			e_desc_idx_stride idx_stride = e_desc_idx_stride::idx_stride_8,
			bool cache_swizzle = false);
	};
}