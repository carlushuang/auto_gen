#pragma once

#include <stdarg.h>
#include <CL/cl.h>
#include <vector>
#include <memory>


class RuntimeEngineOCL : public RuntimeEngineBase{
public:
	virtual E_ReturnState Init();
	virtual E_ReturnState Destroy();

	virtual void * AllocDeviceMem(int bytes);
	virtual void * AllocPinnedMem(int bytes);
	virtual E_ReturnState Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream);
	virtual void Free(void * mem);

	virtual int GetDeviceNum() const;
	virtual DeviceBase * GetDevice(int index);

	cl_platform_id      platform;
	cl_context 			context;

	std::vector<std::unique_ptr<DeviceBase>> devices;
private:
	RuntimeEngineOCL(){}
	static RuntimeEngineOCL INSTANCE;

	friend class RuntimeEngine;
};

class OCLBinaryCompiler : public CompilerBase<RuntimeEngineOCL>{
public:
	OCLBinaryCompiler(RuntimeEngineBase * runtime_ctl_) : CompilerBase( dynamic_cast<RuntimeEngineOCL*>(runtime_ctl_)){}
	~OCLBinaryCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};

class OCLASMCompiler : public CompilerBase<RuntimeEngineOCL>{
public:
	OCLASMCompiler(RuntimeEngineBase * runtime_ctl_) : CompilerBase(dynamic_cast<RuntimeEngineOCL*>(runtime_ctl_)){}
	~OCLASMCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};

class OCLCCompiler : public CompilerBase<RuntimeEngineOCL>{
public:
	OCLCCompiler(RuntimeEngineBase * runtime_ctl_) : CompilerBase(dynamic_cast<RuntimeEngineOCL*>(runtime_ctl_)){}
	~OCLCCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};
