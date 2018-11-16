#pragma once
#include <string>
#include <string>
#include <iostream>
#include "BasicClass.h"

enum MEMCPY_TYPE{
	MEMCPY_HOST_TO_DEV,
	MEMCPY_DEV_TO_HOST,
	MEMCPY_DEV_TO_DEV,
};

#define OBJ_DECLARE(clz)				\
public:								\
	clz(void * obj):obj_handler(obj){}	\
	virtual ~clz(){}					\
	void * object(){return obj_handler;} \
protected:				\
	void * obj_handler;

struct KernelObject{
	OBJ_DECLARE(KernelObject)
public:
	struct KArg{
		// make a assumption that all kernel args cares about the bytes in store, not the type
		unsigned char * content;
		int bytes;
		KArg():content(nullptr), bytes(0){}
		~KArg(){
			if(content)
				delete [] content;
		}
		template<typename T>
		void Set(T * value){
			bytes = sizeof(T);
			content = new unsigned char[bytes];
			memcpy((void*)content, (void*)value, bytes);
		}
	};
	template<typename T>
	void SetKernelArg(T * value){
		std::unique_ptr<KArg> k_arg(new KArg);
		k_arg->Set(value);
		kernel_args.push_back(std::move(k_arg));
	}
	std::vector<std::unique_ptr<KArg>> kernel_args;
};

struct CodeObject{
	OBJ_DECLARE(CodeObject)
public:
	virtual E_ReturnState Serialize(std::ostream & os ){}
	virtual KernelObject * CreateKernelObject(const char * kernel_name){}
};

class DeviceBase;

template <typename RUNTIME_TYPE>
class CompilerBase {
public:
	CompilerBase(RUNTIME_TYPE * runtime_ctl_) : runtime_ctl(runtime_ctl_){}
	virtual ~CompilerBase(){}
	virtual std::string GetBuildOption() = 0;
	//virtual std::string GetCompiler() = 0;

	virtual CodeObject * operator()(const unsigned char * content, int bytes, DeviceBase * dev) = 0;

protected:
	RUNTIME_TYPE * runtime_ctl;
};

static inline E_ReturnState GetFileContent(const char * file_name, unsigned char ** content, int * bytes){
	FILE * fp = fopen(file_name, "rb");
	if (fp == NULL)
	{
		std::cerr<<"can't open bin file: "<<file_name<<std::endl;
		return E_ReturnState::FAIL;
	}

	size_t bin_size;
	fseek(fp, 0, SEEK_END);
	bin_size = ftell(fp);
	rewind(fp);

	unsigned char * bin_content = new unsigned char[bin_size];
	fread(bin_content, 1, bin_size, fp);
	fclose(fp);

	*content = bin_content;
	*bytes = bin_size;

	return E_ReturnState::SUCCESS;
}

struct DispatchParam{
	int local_size[3];
	int global_size[3];

	KernelObject * kernel_obj;
};

class StreamBase {
	OBJ_DECLARE(StreamBase)
public:
	StreamBase(void * stream_obj, DeviceBase * dev_, bool is_async_):
			StreamBase(stream_obj){
				dev = dev_;
				is_async = is_async_;
			}
	virtual E_ReturnState SetupDispatch(DispatchParam * dispatch_param) = 0;
	virtual E_ReturnState Launch(int iter) = 0;
	virtual E_ReturnState Wait() = 0;
protected:
	DeviceBase * dev;
	bool is_async;
};

typedef struct DeviceInfoType
{
	unsigned int DeviceIdx;
	std::string DeviceName;
	unsigned int ComputeUnitNum;
	unsigned int ProcessingElementNum;
	unsigned long GlobalMemSize;
	size_t ConstBufSize;
	size_t LocalMemSize;
	std::string RuntimeVersion;
	double CoreFreq;
	double Fp32Flops;
} DeviceInfo;

static inline int DumpDeviceInfo(DeviceInfo * dev_info){
	printf("#####################   Device Info  #####################\n");
	printf("# Device Id: %d\n", dev_info->DeviceIdx);
	printf("# Device Name: %s\n", dev_info->DeviceName.c_str());
	printf("# CU num: %d\n", dev_info->ComputeUnitNum);
	printf("# PE num: %d\n", dev_info->ProcessingElementNum);
	printf("# Clock Frequency: %.1f GHz\n", dev_info->CoreFreq * 1e-9);
	printf("# Performence: %.1f Tflops\n", dev_info->Fp32Flops * 1e-12);
	printf("# Global Memory: %.1fG Byte\n", dev_info->GlobalMemSize * 1e-9);
	printf("##########################################################\n");
}

class DeviceBase {
	OBJ_DECLARE(DeviceBase)
public:
	virtual StreamBase * CreateStream(bool is_async=false){}
	virtual void GetDeviceInfo(DeviceInfo * dev_info){}
};

// Base class for runtime control, aka context. should be singleton
class RuntimeEngineBase {
public:
	virtual E_ReturnState Init() = 0;
	virtual E_ReturnState Destroy() = 0;

	virtual void * AllocDeviceMem(int bytes){}
	virtual void * AllocPinnedMem(int bytes){}
	virtual E_ReturnState Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream) = 0;
	virtual void Free(void * mem) = 0;

	virtual int GetDeviceNum() const  = 0;
	virtual DeviceBase * GetDevice(int index) = 0;
};

class RuntimeEngine;
#ifdef RUNTIME_OCL
#include "RuntimeEngineOCL.h"
#endif
#ifdef RUNTIME_HSA
#include "RuntimeEngineHSA.h"
#endif

class RuntimeEngine{
public:
	static RuntimeEngineBase* Get(std::string name){
#ifdef RUNTIME_OCL
		if(name == "opencl" || name == "OpenCL" || name == "ocl")
			return &RuntimeEngineOCL::INSTANCE;
#endif
#ifdef RUNTIME_HSA
		if(name == "hsa" || name == "HSA")
			return &RuntimeEngineHSA::INSTANCE;
#endif
		std::cerr<<"N/A runtime ctrl name "<<name<<std::endl;
		return nullptr;
	}
	static void Destroy(RuntimeEngineBase * runtime_ctl){
		if(runtime_ctl)
			runtime_ctl->Destroy();
	}
};