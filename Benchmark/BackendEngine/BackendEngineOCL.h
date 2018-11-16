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
	BackendEngineOCL(){}
	static BackendEngineOCL INSTANCE;

	virtual E_ReturnState Init();
	virtual E_ReturnState Destroy();

	friend class BackendEngine;
};

class OCLBinaryCompiler : public CompilerBase<BackendEngineOCL>{
public:
	OCLBinaryCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase( dynamic_cast<BackendEngineOCL*>(runtime_ctl_)){}
	~OCLBinaryCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};

class OCLASMCompiler : public CompilerBase<BackendEngineOCL>{
public:
	OCLASMCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(dynamic_cast<BackendEngineOCL*>(runtime_ctl_)){}
	~OCLASMCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};

class OCLCCompiler : public CompilerBase<BackendEngineOCL>{
public:
	OCLCCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(dynamic_cast<BackendEngineOCL*>(runtime_ctl_)){}
	~OCLCCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};
