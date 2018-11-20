#include "hsa.h"
#include "hsa_ext_amd.h"
#include <vector>


class BackendEngineHSA : public BackendEngineBase{
public:
	virtual void * AllocDeviceMem(int bytes, DeviceBase * dev = nullptr);
	virtual void * AllocPinnedMem(int bytes, DeviceBase * dev = nullptr);
	virtual E_ReturnState Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream);
	virtual void Free(void * mem);

	virtual int GetDeviceNum() const;
	virtual DeviceBase * GetDevice(int index);
	virtual BackendEngineType Type() const {return BACKEND_ENGINE_HSA;}


	std::vector<std::unique_ptr<DeviceBase>> devices;
private:
	BackendEngineHSA(){}

	virtual E_ReturnState Init();
	virtual E_ReturnState Destroy();

	friend class BackendEngine;
	INSTANCE_DECLARE(BackendEngineHSA, BackendEngineBase)
};

class HSABinaryCompiler : public CompilerBase{
public:
	~HSABinaryCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
	INSTANCE_DECLARE(HSABinaryCompiler, CompilerBase)
private:
	HSABinaryCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase(runtime_ctl_){}
};

class HSAASMCompiler : public CompilerBase{
public:
	~HSAASMCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
	INSTANCE_DECLARE(HSAASMCompiler, CompilerBase)
private:
	HSAASMCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase( runtime_ctl_){}
};