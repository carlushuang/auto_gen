#pragma once

#include <stdarg.h>
#include <CL/cl.h>
#include <vector>
#include <memory>


class BackendEngineOCL : public BackendEngineBase{
public:
	~BackendEngineOCL(){
		if(Inited())
			Destroy();
	}
	virtual void * AllocDeviceMem(int bytes, DeviceBase * dev = nullptr);
	virtual void * AllocPinnedMem(int bytes, DeviceBase * dev = nullptr);
	virtual E_ReturnState Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream);
	virtual void Free(void * mem);

	virtual int GetDeviceNum() const;
	virtual DeviceBase * GetDevice(int index);
	virtual BackendEngineType Type() const {return BACKEND_ENGINE_OCL;}

	cl_platform_id      platform;
	cl_context 			context;

	std::vector<std::unique_ptr<DeviceBase>> devices;
private:
	BackendEngineOCL(){}

	virtual E_ReturnState Init();
	virtual E_ReturnState Destroy();

	friend class BackendEngine;
	INSTANCE_DECLARE(BackendEngineOCL, BackendEngineBase)
};

class OCLBinaryCompiler : public CompilerBase{
public:
	
	~OCLBinaryCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);

	INSTANCE_DECLARE(OCLBinaryCompiler, CompilerBase)
private:
	OCLBinaryCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(runtime_ctl_){}
};

class OCLASMCompiler : public CompilerBase{
public:
	~OCLASMCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
	INSTANCE_DECLARE(OCLASMCompiler, CompilerBase)
private:
	OCLASMCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(runtime_ctl_){}
};

class OCLCCompiler : public CompilerBase{
public:
	~OCLCCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
	INSTANCE_DECLARE(OCLCCompiler, CompilerBase)
private:
	OCLCCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(runtime_ctl_){}
};
