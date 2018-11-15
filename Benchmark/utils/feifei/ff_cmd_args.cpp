
#include <string.h>
#include <iostream>

#include "ff_log.h"
#include "ff_cmd_args.h"

namespace feifei 
{
	CmdArgs::CmdArgs(int argc, char *argv[])
	{
		argsNum = 0;
		argsMap = new std::map<E_ArgId, T_CmdArg*>();
		initCmdArgs();
		HelpText();

		paserCmdArgs(argc, argv);
	}

	void CmdArgs::HelpText()
	{
		std::map<E_ArgId, T_CmdArg *>::iterator it;

		for (it = argsMap->begin(); it != argsMap->end(); it++)
		{
			if (it->second->shortName != '\0')
				printf("-%c", it->second->shortName);

			if (it->second->longName != "")
				printf("  --%s", it->second->longName.c_str());

			if (it->second->helpText != "")
				printf(": %s", it->second->helpText.c_str());

			printf(".\n");
		}
	}

	void * CmdArgs::GetOneArg(E_ArgId id)
	{
		std::map<E_ArgId, T_CmdArg *>::iterator it;
		it = argsMap->find(id);

		if (it == argsMap->end())
		{
			return nullptr;
		}

		return getOneArgValue(it->second);
	}

	/*
	* 添加默认参数和命令
	*/
	void CmdArgs::initCmdArgs()
	{
		addOneArg(CMD_ARG_DEVICE, E_DataType::Int, "-1", 'd', "device", "specify a device.");
		addOneArg(CMD_ARG_A, E_DataType::Int, "-2", 'a', "test_a", "specify a device.");
		addOneArg(CMD_ARG_B, E_DataType::Int, "-3", 'b', "test_b");
		addOneArg(CMD_ARG_C, E_DataType::Int, "-4", 'c');
	}
	void CmdArgs::addOneArg(E_ArgId id, E_DataType dType, std::string defaultVal, char sName, std::string lName, std::string tHelp)
	{
		T_CmdArg *arg = new T_CmdArg;

		arg->id = id;
		arg->type = dType;
		arg->value = defaultVal;
		arg->shortName = sName;
		arg->longName = lName;
		arg->helpText = tHelp;

		switch (dType)
		{
		case E_DataType::Int:	arg->iValue = atoi(defaultVal.c_str()); break;
		case E_DataType::Float:	arg->fValue = atof(defaultVal.c_str()); break;
		}

		argsMap->insert(std::pair<E_ArgId, T_CmdArg*>(id, arg));
		argsNum++;
	}

	/*
	 * 解析所有命令行参数
	 */
	void CmdArgs::paserCmdArgs(int argc, char *argv[])
	{
		for (int i = 0; i < argc; i++)
		{
			if ((argv[i][0] == '-')&&(argv[i][1] == '-'))
			{
				setOneArg(std::string(&argv[i][2]), std::string(argv[i + 1]));
			}
			else if (argv[i][0] == '-')
			{
				setOneArg(argv[i][1], std::string(argv[i + 1]));
			}
		}
	}

	/*
	* 根据命令行找到一个对应参数 
	*/
	E_ReturnState CmdArgs::setOneArg(char sName, std::string value)
	{
		std::map<E_ArgId, T_CmdArg *>::iterator it;

		for (it = argsMap->begin(); it != argsMap->end(); it++)
		{
			if (it->second->shortName == sName)
				break;
		}

		if (it == argsMap->end())
		{
			WARN("no such param -%c.", sName);
			return E_ReturnState::FAIL;
		}

		return setOneArgValue(it->second, value);
	}
	E_ReturnState CmdArgs::setOneArg(std::string lName, std::string value)
	{
		std::map<E_ArgId, T_CmdArg *>::iterator it;

		for (it = argsMap->begin(); it != argsMap->end(); it++)
		{
			if (it->second->longName == lName)
				break;
		}

		if (it == argsMap->end())
		{
			WARN("no such param --%s.", lName.c_str());
			return E_ReturnState::FAIL;
		}

		return setOneArgValue(it->second, value);
	}
	
	/*
	* 根据命令行,设置/获取一个参数的值
	* 参数类型在初始化阶段设置
	*/
	E_ReturnState CmdArgs::setOneArgValue(T_CmdArg * arg, std::string value)
	{
		switch (arg->type)
		{
		case E_DataType::Int: 
			arg->iValue = atoi(value.c_str()); 
			break;
		case E_DataType::Float:
			arg->fValue = atof(value.c_str());
			break;
		case E_DataType::String:
			arg->sValue = value;
			break;
		default: 
			break;
		}
		return E_ReturnState::SUCCESS;
	}
	void * CmdArgs::getOneArgValue(T_CmdArg * arg)
	{
		switch (arg->type)
		{
		case E_DataType::Int:		return &(arg->iValue);
		case E_DataType::Float:		return &(arg->fValue);
		case E_DataType::String:	return &(arg->sValue);
		default:					return nullptr;
		}
	}
}
