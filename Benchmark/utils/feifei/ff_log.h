#pragma once

#include "BasicClass.h"

#define	INFO(fmt,...)		feifei::print_format_info(fmt,##__VA_ARGS__)
#define WARN(fmt,...)		feifei::print_format_warn(__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define ERR(fmt,...)		feifei::print_format_err(__FILE__,__LINE__,fmt,##__VA_ARGS__)
#define FATAL(fmt,...)		feifei::print_format_fatal(__FILE__,__LINE__,fmt,##__VA_ARGS__)

#define NEW_LOG(file)		feifei::init_log_file(file)
#define	LOG(file,fmt,...)	feifei::write_format_to_file(file,fmt,##__VA_ARGS__)

namespace feifei
{
	/************************************************************************/
	/* 屏幕输出																*/
	/************************************************************************/
	extern void print_format_info(const char * format, ...);
	extern void print_format_info(std::string msg, ...);
	extern void print_format_warn(const char *file, int line, const char * format, ...);
	extern void print_format_warn(const char *file, int line, std::string msg, ...);
	extern E_ReturnState print_format_err(const char *file, int line, const char * format, ...);
	extern E_ReturnState print_format_err(const char *file, int line, std::string msg, ...);
	extern void print_format_fatal(const char *file, int line, const char * format, ...);
	extern void print_format_fatal(const char *file, int line, std::string msg, ...);

	/************************************************************************/
	/* 文件输出																*/
	/************************************************************************/
	extern std::ofstream * init_log_file(const char * file_name);
	extern void write_format_to_file(std::ofstream * file, const char * format, ...);
}
