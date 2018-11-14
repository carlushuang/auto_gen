#pragma once

#include "ff_basic.h"

namespace feifei
{
	typedef enum ArgTypeEnum
	{

	} E_ArgType;

	typedef struct CmdArgType
	{
		std::string longName;
		char shortName;
		std::string value;
		int iValue;
		double fValue;
		E_DataType type;
		std::string helpText;
	} T_CmdArg;

	class CmdArgs
	{
	public:
		CmdArgs(int argc, char *argv[]);

		

	private:
		int argsNum = 0;
		std::map<char, T_CmdArg*> * argsMap;
		
		void initCmdArg();
		void addOneArg(char sName, std::string lName, std::string defaultVal, E_DataType dType, std::string tHelp = "");
		//void addCmdArg();
	};
}
