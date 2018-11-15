
#include "RuntimeEngine.h"
#include <iostream>
#include <string.h>
#include <stdio.h>

#define MAX_LOG_SIZE (16384)

#define CHECK_OCL_ERR(status, msg) \
	do{	\
		if(status != CL_SUCCESS){	\
			std::cerr<<"ERROR "<< __FILE__ << ":" << __LINE__<<" status:"<<status<<", "<<msg<<std::endl;	\
			return E_ReturnState::FAIL;	\
		}\
	}while(0)


class OCLDevice;
class OCLStream : public StreamBase{
public:
	OCLStream(void * stream_obj, DeviceBase * dev_, bool is_async_) :
			StreamBase(stream_obj, dev_, is_async_)
	{}
	~OCLStream(){
		cl_command_queue queue = (cl_command_queue)object();
		if(queue)
			clReleaseCommandQueue(queue);
	}

	virtual E_ReturnState SetupDispatch(DispatchParam * dispatch_param){
		cl_int status;
		KernelObject * kernel_obj = dispatch_param->kernel_obj;
		this->kernel = (cl_kernel)kernel_obj->object();
		if(kernel_obj->kernel_args.size() == 0){
			std::cerr<<"SetupDispatch with empty kernel args, ignore"<<std::endl;
			return E_ReturnState::FAIL;
		}
		for(int i=0;i<kernel_obj->kernel_args.size();i++){
			KernelObject::KArg * k_arg = kernel_obj->kernel_args[i].get();
			status = clSetKernelArg(kernel, i, (size_t)k_arg->bytes, (const void*)k_arg->content);
			if(status != CL_SUCCESS){
				std::cerr<<"clSetKernelArg fail, idx:"<<i<<std::endl;
				return E_ReturnState::FAIL;
			}
		}
		this->local_size[0] = dispatch_param->local_size[0];
		this->local_size[1] = dispatch_param->local_size[1];
		this->local_size[2] = dispatch_param->local_size[2];
		this->global_size[0] = dispatch_param->global_size[0];
		this->global_size[1] = dispatch_param->global_size[1];
		this->global_size[2] = dispatch_param->global_size[2];
		num_dims = 1;
		if(this->local_size[1])
			num_dims = 2;
		if(this->local_size[2])
			num_dims = 3;
		return E_ReturnState::SUCCESS;
	}
	virtual E_ReturnState Launch(int iter){
		cl_int status;
		cl_command_queue queue = (cl_command_queue)object();
		for(int i=0;i<iter;i++){
			status = clEnqueueNDRangeKernel(queue, kernel, num_dims, NULL,
                                    global_size, local_size,
                                    0, NULL, NULL);
			if(status != CL_SUCCESS){
				std::cerr<<"clEnqueueNDRangeKernel fail, loop:"<<i<<std::endl;
				return E_ReturnState::FAIL;
			}
		}
		return E_ReturnState::SUCCESS;
	}
	virtual E_ReturnState Wait() {
		cl_command_queue queue = (cl_command_queue)object();
		clFlush(queue);
		return E_ReturnState::SUCCESS;
	}

	cl_kernel kernel;
	cl_uint num_dims;
	size_t local_size[3];
	size_t global_size[3];
};

class OCLDevice : public DeviceBase{
public:
	OCLDevice(void * obj, RuntimeEngineOCL * re):DeviceBase(obj), engine(re){}
	~OCLDevice(){}
	virtual StreamBase * CreateStream(bool is_async=false){
		cl_queue_properties q_prop = 0;
		if(is_async)
			q_prop |= CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
		cl_int status;
		cl_command_queue queue = clCreateCommandQueue(engine->context, (cl_device_id)this->object(), q_prop, &status);
		if(status != CL_SUCCESS){
			std::cerr<<"Failed clCreateCommandQueue"<<std::endl;
			return nullptr;
		}
		OCLStream * stream = new OCLStream(queue, this, is_async);
		return stream;
	}
	RuntimeEngineOCL * engine;
};

struct OCLCodeObject: public CodeObject {
	OCLCodeObject(void * obj):CodeObject(obj){}
	~OCLCodeObject(){
		cl_program program = (cl_program)object();
		if(program)
			clReleaseProgram(program);
	}
	virtual KernelObject * CreateKernelObject(const char * kernel_name){
		cl_int status;
		cl_program program = (cl_program)object();
		cl_kernel kernel = clCreateKernel(program, kernel_name, &status);
		if(status != CL_SUCCESS){
			std::cerr<<"fail to clCreateKernel"<<std::endl;
			return nullptr;
		}

		KernelObject * kernel_obj = new KernelObject(kernel);
		return kernel_obj;
	}
};

static void toCLDeviceID(std::vector<std::unique_ptr<DeviceBase>> & dev_vec,  std::vector<cl_device_id> & cl_devices){
	for(int i=0;i<dev_vec.size();i++){
		OCLDevice * cl_dev = (OCLDevice*)(dev_vec[i].get());
		cl_devices.push_back( (cl_device_id)cl_dev->object());
	}
}

E_ReturnState RuntimeEngineOCL::Init(){
	cl_int status = CL_SUCCESS;
	cl_uint num_platforms;
	platform = NULL;
	status = clGetPlatformIDs(0, NULL, &num_platforms);
	CHECK_OCL_ERR(status, "clGetPlatformIDs failed.");
	if (0 < num_platforms)
	{
		// seach AMD OCL
		cl_platform_id* platforms = new cl_platform_id[num_platforms];
		status = clGetPlatformIDs(num_platforms, platforms, NULL);
		CHECK_OCL_ERR(status, "clGetPlatformIDs failed.");
		char platformName[100];
		for (unsigned i = 0; i < num_platforms; ++i)
		{
			status = clGetPlatformInfo(
							platforms[i],
							CL_PLATFORM_VENDOR,
							sizeof(platformName),
							platformName,
							NULL);
			CHECK_OCL_ERR(status, "clGetPlatformInfo failed.");
			platform = platforms[i];
			if (!strcmp(platformName, "Advanced Micro Devices, Inc."))
			{
				std::cout << "Platform found : " << platformName << "\n";
				break;
			}
		}
		delete[] platforms;
	}
	if(NULL == platform)
	{
		std::cout << "NULL platform found so Exiting Application.";
		return E_ReturnState::FAIL;
	}
#define MAX_DEVICES 16
	cl_device_id cl_devices[MAX_DEVICES];
	cl_uint num_devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_DEFAULT, MAX_DEVICES, cl_devices, &num_devices);
	CHECK_OCL_ERR(status, "clGetDeviceIDs failed.");

	context = clCreateContext(0, num_devices, cl_devices, NULL, NULL, &status);
	CHECK_OCL_ERR(status, "clCreateContext failed.");

	for(int i=0;i<num_devices;i++){
		OCLDevice *dev = new OCLDevice(cl_devices[i], this);

		std::unique_ptr<DeviceBase> dev_ptr(dev);
		devices.push_back(std::move(dev_ptr));
	}
	return E_ReturnState::SUCCESS;
}
int RuntimeEngineOCL::GetDeviceNum() const{
	return devices.size();
}
DeviceBase * RuntimeEngineOCL::GetDevice(int index){
	if(index > (devices.size()-1))
		return nullptr;
	return devices[index].get();
}
void * RuntimeEngineOCL::AllocDeviceMem(int bytes){
	cl_int err;
	cl_mem mem = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, &err);
	if(err != CL_SUCCESS){
		std::cerr<<"fail to clCreateBuffer"<<std::endl;
		return NULL;
	}
	return (void*)mem;
}
void * RuntimeEngineOCL::AllocPinnedMem(int bytes){
	return NULL;
}
E_ReturnState RuntimeEngineOCL::Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream){
	cl_command_queue command_queue = (cl_command_queue)stream->object();
	cl_int rtn;
	switch(memcpy_type){
		case MEMCPY_HOST_TO_DEV:
			rtn = clEnqueueWriteBuffer(command_queue, (cl_mem)dst, CL_TRUE/*blocking*/, 0, bytes, src, 0, NULL, NULL);
			if(rtn != CL_SUCCESS){
				std::cerr<<"Fail clEnqueueWriteBuffer"<<std::endl;
			}
		break;
		case MEMCPY_DEV_TO_HOST:
			rtn = clEnqueueReadBuffer(command_queue, (cl_mem)src, CL_TRUE/*blocking*/, 0, bytes, dst, 0, NULL, NULL);
			if(rtn != CL_SUCCESS){
				std::cerr<<"Fail clEnqueueReadBuffer"<<std::endl;
			}
		break;
		case MEMCPY_DEV_TO_DEV:
		break;
		default:
		break;
	}
}
void RuntimeEngineOCL::Free(void * mem){
	clReleaseMemObject((cl_mem)mem);
}

/***************************************************************************************/

std::string OCLBinaryCompiler::GetBuildOption(){
	/*
	 * the program was created using clCreateProgramWithBinary and options is a NULL pointer,
	 * the program will be built as if options were the same as when the program binary was originally built.
	 * if the program was created using clCreateProgramWithBinary and options string contains
	 * the behavior is implementation defined.
	 */
	return "";
}

// create one for all
CodeObject * OCLBinaryCompiler::operator()(const unsigned char * content, int bytes, DeviceBase * dev){
	cl_program program;
	std::vector<cl_device_id> cl_devices;
	OCLDevice * cl_dev = (OCLDevice*)dev;
	cl_devices.push_back((cl_device_id)cl_dev->object());
	//toCLDeviceID(runtime_ctl->devices, cl_devices);

	OCLCodeObject * code_obj = nullptr;
	//std::string build_opt = GetBuildOption();

	cl_context ctx = runtime_ctl->context;
	int num_devices = 1;

	cl_int *per_status = new cl_int[num_devices];
	size_t * bin_len = new size_t[num_devices];
	for(int i=0;i<num_devices;i++)
		bin_len[i] = (size_t)bytes;
	unsigned char ** bin_data = new unsigned char *[num_devices];
	for(int i=0;i<num_devices;i++)
		bin_data[i] = (unsigned char*)content;

	cl_int status;
	program = clCreateProgramWithBinary(ctx, num_devices, cl_devices.data(), bin_len, 
			(const unsigned char**)bin_data, per_status, &status);
	for (int i = 0; i < num_devices; i++){
		if(per_status[i] != CL_SUCCESS){
			std::cerr<<"clCreateProgramWithBinary faild with binary for dev:"<<i<<", status:"<<status <<std::endl;
			goto out;
		}
	}
	if(status != CL_SUCCESS){
		std::cerr<<"clCreateProgramWithBinary faild with binary, status:"<<status<<std::endl;
		goto out;
	}

	// ignore build option for from binary source
	status = clBuildProgram(program, num_devices, cl_devices.data(), NULL, NULL, NULL);

	if(status != CL_SUCCESS){
		std::cerr<<"clBuildProgram faild"<<std::endl;
		clReleaseProgram(program);
		goto out;
	}

	code_obj = new OCLCodeObject(program);

out:
	delete [] per_status;
	delete [] bin_data;
	delete [] bin_len;
	return (CodeObject*)code_obj;
}

std::string OCLCCompiler::GetBuildOption(){
	return "";
}
CodeObject * OCLCCompiler::operator()(const unsigned char * content, int bytes, DeviceBase * dev){
	cl_context ctx = runtime_ctl->context;
	size_t src_len[1] = {bytes};
	OCLDevice * cl_dev = (OCLDevice*)dev;
	cl_device_id cl_devices[1] = {(cl_device_id)cl_dev->object()};
	cl_int status;
	cl_program program = clCreateProgramWithSource(ctx, 1,
										(const char**)&content,
										src_len, &status);
	if(status != CL_SUCCESS){
		std::cerr << "Failed to create CL program from source, status:"<<status << std::endl;
		return NULL;
	}
	std::string build_opt = GetBuildOption();

	status = clBuildProgram(program, 1, cl_devices, build_opt.c_str(), NULL, NULL);
	if (status != CL_SUCCESS)
	{
		// Determine the reason for the error
		char buildLog[16384];
		clGetProgramBuildInfo(program, cl_devices[0], CL_PROGRAM_BUILD_LOG,
								sizeof(buildLog), buildLog, NULL);

		std::cerr << "Error in kernel: " << std::endl;
		std::cerr << buildLog;
		clReleaseProgram(program);
		return NULL;
	}

	OCLCodeObject * code_obj = new OCLCodeObject(program);
	return (CodeObject*)code_obj;
}