#include "hsa.h"
#include "hsa_ext_amd.h"
#include <vector>


class BackendEngineHSA : public BackendEngineBase{
public:
	virtual void * AllocDeviceMem(int bytes);
	virtual void * AllocPinnedMem(int bytes);
	virtual E_ReturnState Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream);
	virtual void Free(void * mem);

	virtual int GetDeviceNum() const;
	virtual DeviceBase * GetDevice(int index);

	std::vector<std::unique_ptr<DeviceBase>> devices;
private:
	BackendEngineHSA(){}
	static BackendEngineHSA INSTANCE;

	virtual E_ReturnState Init();
	virtual E_ReturnState Destroy();

	friend class BackendEngine;
};

class HSABinaryCompiler : public CompilerBase<BackendEngineHSA>{
public:
	HSABinaryCompiler(BackendEngineBase * runtime_ctl_) : CompilerBase( dynamic_cast<BackendEngineHSA*>(runtime_ctl_)){}
	~HSABinaryCompiler(){}
	virtual std::string GetBuildOption();

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev);
};
