
#include <string.h>
#include <fstream>
#include <iostream>
#include <stdarg.h>
#include <time.h>

#ifdef linux
#include <unistd.h>
#include <sys/stat.h>
#endif
#ifdef WIN32
#include <io.h>
#endif

#include "ff_log.h"

namespace feifei 
{
#define		CHAR_BUFF_SIZE		(1024)

	static char log_char_buffer[CHAR_BUFF_SIZE];

	static bool en_log_time = true;
	static bool en_log_file = true;
	static bool en_log_short_file = true;

	/************************************************************************/
	/* 屏幕输出																*/
	/************************************************************************/
	void print_format_info(const char * format, ...)
	{
		printf("[INFO]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(format);
		printf("\n");
	}
	void print_format_info(std::string msg, ...)
	{
		printf("[INFO]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		printf("\n");
	}

	void print_format_warn(const char *file, int line, const char * format, ...)
	{
		printf("[WARNING]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(format);
		if (en_log_file)
		{
			char * p = (char * )file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
	}
	void print_format_warn(const char *file, int line, std::string msg, ...)
	{
		printf("[WARNING]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
	}

	E_ReturnState print_format_err(const char *file, int line, const char * format, ...)
	{
		printf("[ERROR]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(format);
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		return E_ReturnState::FAIL;
	}
	E_ReturnState print_format_err(const char *file, int line, std::string msg, ...)
	{
		printf("[ERROR]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		return E_ReturnState::FAIL;
	}

	void print_format_fatal(const char *file, int line, const char * format, ...)
	{
		printf("[FATAL]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(format);
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n"); 
		exit(EXIT_FAILURE);
	}
	void print_format_fatal(const char *file, int line, std::string msg, ...)
	{
		printf("[FATAL]");
		if (en_log_time == true)
		{
			time_t t = time(0);
			memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
			strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%H:%M:%S]", localtime(&t));
			printf("%s", log_char_buffer);
		}
		printf(msg.c_str());
		if (en_log_file)
		{
			char * p = (char *)file;
			if (en_log_short_file == true)
			{
				p = strrchr(p, '/');
				p++;
			}
			printf(" @%s:%d", p, line);
		}
		printf("\n");
		exit(EXIT_FAILURE);
	}
	
	/************************************************************************/
	/* 文件输出																*/
	/************************************************************************/
	std::ofstream * init_log_file(const char * file_name)
	{
		/*if (access(".//log", F_OK) == -1)
		{
			::mkdir(".//log", 0777);
		}*/

		time_t t = time(0);
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		strftime(log_char_buffer, CHAR_BUFF_SIZE, "_%F_%H-%M-%S.log", localtime(&t));
		//std::string fname = ".//log//" + std::string(file_name) + log_char_buffer;
		std::string fname = ".//" + std::string(file_name) + log_char_buffer;
		
		std::ofstream * file;
		file = new std::ofstream(fname, std::ios::out | std::ios::trunc);

		if (!file->is_open())
		{
			WARN("can't init log file" + fname);
			return nullptr;
		}

		return file;
	}
	
	void write_format_to_file(std::ofstream * file, const char * format, ...)
	{
		int cn;

		if (file == nullptr)
		{
			WARN("can't open log file");
			return;
		}
		if (!file->is_open())
		{
			WARN("can't open log file");
			return;
		}
		
		time_t t = time(0);
		memset(log_char_buffer, 0, CHAR_BUFF_SIZE);
		cn = strftime(log_char_buffer, CHAR_BUFF_SIZE, "[%F_%T]", localtime(&t));

		va_list args;
		va_start(args, format);
		printf(format, args);
		cn += vsprintf(log_char_buffer+cn, format, args);
		va_end(args);
		cn += sprintf(log_char_buffer + cn, "\n");

		file->write(log_char_buffer, cn);
		file->flush();
	}
}
