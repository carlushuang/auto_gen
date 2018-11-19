
#include "BackendEngine.h"
#include <iostream>
#include <string.h>
#include <stdio.h>

#define		GPU_CORE_FREQ_HZ		(1.5e9)
#define		GPU_SE_NUM				(4)
#define		GPU_CU_NUM_PER_SE		(16)
#define		GPU_SIMD_NUM_PER_CU		(4)
#define		GPU_ALU_NUM_PER_SIMD	(16)
#define		GPU_ALU_NUM				(GPU_ALU_NUM_PER_SIMD*GPU_SIMD_NUM_PER_CU*GPU_CU_NUM_PER_SE*GPU_SE_NUM)
#define		GPU_CALCULATION			(GPU_ALU_NUM * GPU_CORE_FREQ_HZ)

#define MAX_LOG_SIZE (16384)

#define CHECK_OCL_ERR(status, msg) \
	do{	\
		if(status != CL_SUCCESS){	\
			std::cerr<<"ERROR "<< __FILE__ << ":" << __LINE__<<" status:"<<status<<", "<<msg<<std::endl;	\
			return E_ReturnState::FAIL;	\
		}\
	}while(0)


static void DumpCLBuildLog(cl_program program, cl_device_id device){
	size_t len;
	char buffer[2048];
	std::cout<<"_____________________________cl_build_log"<<std::endl;
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
	std::cout<<buffer<<std::endl;
	std::cout<<"_____________________________cl_build_log"<<std::endl;
}

BackendEngineOCL BackendEngineOCL::INSTANCE;

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

#define CL_DEV_INFO_P(ptr, size, param, id) \
	do{				\
		cl_int status = clGetDeviceInfo(id, param, size, ptr, NULL); \
		if(status != CL_SUCCESS){		\
			std::cerr<<"Failt to clGetDeviceInfo("#param")"<<std::endl; \
			return;	\
		}	\
	}while(0)

#define CL_DEV_INFO(store, param, id) \
		CL_DEV_INFO_P(&store, sizeof(store), param, id)
	

class OCLDevice : public DeviceBase{
public:
	OCLDevice(void * obj, BackendEngineOCL * re):DeviceBase(obj), engine(re){}
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
	virtual void GetDeviceInfo(DeviceInfo * dev_info){
		if(!dev_info)
			return ;
		cl_device_id dev_id = (cl_device_id)object();
		cl_uint v_uint;
		cl_ulong v_ulong;
		char v_char[1024];

		dev_info->DeviceIdx = this->index;

		CL_DEV_INFO(v_uint, CL_DEVICE_MAX_COMPUTE_UNITS, dev_id);
		dev_info->ComputeUnitNum = v_uint;

		CL_DEV_INFO_P(v_char, sizeof(v_char), CL_DEVICE_NAME, dev_id);
		dev_info->DeviceName = v_char;

		CL_DEV_INFO(v_uint, CL_DEVICE_MAX_CLOCK_FREQUENCY, dev_id);
		dev_info->CoreFreq =v_uint * 1e6;

		CL_DEV_INFO(v_ulong, CL_DEVICE_GLOBAL_MEM_SIZE, dev_id);
		dev_info->GlobalMemSize = v_ulong;

		dev_info->ProcessingElementNum = dev_info->ComputeUnitNum * GPU_SIMD_NUM_PER_CU * GPU_ALU_NUM_PER_SIMD;
		dev_info->Fp32Flops = dev_info->ProcessingElementNum * dev_info->CoreFreq * 2;
	}
	BackendEngineOCL * engine;
	int index;
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
	virtual E_ReturnState Serialize(std::ostream & os ){
		cl_int status;
		cl_program program = (cl_program)object();
		size_t binary_size;
		status = clGetProgramInfo(program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binary_size, NULL);
		if(status != CL_SUCCESS){
			std::cerr<<"clGetProgramInfo Fail"<<std::endl;
			return E_ReturnState::FAIL;
		}
		unsigned char * binary_data = new unsigned char[binary_size];
		unsigned char* src[1] = { binary_data };
		status = clGetProgramInfo(program, CL_PROGRAM_BINARIES, sizeof(src), &src, nullptr);
		if(status != CL_SUCCESS){
			std::cerr<<"clGetProgramInfo Fail to dump binary"<<std::endl;
			delete [] binary_data;
			return E_ReturnState::FAIL;
		}

		os.write((const char*)binary_data, binary_size);
		os.flush();
		delete [] binary_data;
		return E_ReturnState::SUCCESS;
	}
};

static void toCLDeviceID(std::vector<std::unique_ptr<DeviceBase>> & dev_vec,  std::vector<cl_device_id> & cl_devices){
	for(int i=0;i<dev_vec.size();i++){
		OCLDevice * cl_dev = (OCLDevice*)(dev_vec[i].get());
		cl_devices.push_back( (cl_device_id)cl_dev->object());
	}
}

E_ReturnState BackendEngineOCL::Init(){
	cl_int status = CL_SUCCESS;
	cl_uint num_platforms;
	platform = NULL;

	// Ignore multiple init
	if(Inited())
		return E_ReturnState::SUCCESS;

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
		dev->index = i;
		std::unique_ptr<DeviceBase> dev_ptr(dev);
		devices.push_back(std::move(dev_ptr));
	}
	this->inited = true;
	return E_ReturnState::SUCCESS;
}

E_ReturnState BackendEngineOCL::Destroy(){
}
int BackendEngineOCL::GetDeviceNum() const{
	return devices.size();
}
DeviceBase * BackendEngineOCL::GetDevice(int index){
	if(index > (devices.size()-1))
		return nullptr;
	return devices[index].get();
}
void * BackendEngineOCL::AllocDeviceMem(int bytes, DeviceBase * dev){
	cl_int err;
	cl_mem mem = clCreateBuffer(context, CL_MEM_READ_WRITE, bytes, NULL, &err);
	if(err != CL_SUCCESS){
		std::cerr<<"fail to clCreateBuffer"<<std::endl;
		return NULL;
	}
	return (void*)mem;
}
void * BackendEngineOCL::AllocPinnedMem(int bytes, DeviceBase * dev){
	return NULL;
}
E_ReturnState BackendEngineOCL::Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream){
	cl_command_queue command_queue = (cl_command_queue)stream->object();
	cl_int status;
	switch(memcpy_type){
		case MEMCPY_HOST_TO_DEV:
			status = clEnqueueWriteBuffer(command_queue, (cl_mem)dst, CL_TRUE/*blocking*/, 0, bytes, src, 0, NULL, NULL);
			if(status != CL_SUCCESS){
				std::cerr<<"Fail clEnqueueWriteBuffer"<<std::endl;
				return E_ReturnState::FAIL;
			}
		break;
		case MEMCPY_DEV_TO_HOST:
			status = clEnqueueReadBuffer(command_queue, (cl_mem)src, CL_TRUE/*blocking*/, 0, bytes, dst, 0, NULL, NULL);
			if(status != CL_SUCCESS){
				std::cerr<<"Fail clEnqueueReadBuffer"<<std::endl;
				return E_ReturnState::FAIL;
			}
		break;
		case MEMCPY_DEV_TO_DEV:
		break;
		default:
		break;
	}
	return E_ReturnState::SUCCESS;
}
void BackendEngineOCL::Free(void * mem){
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
	//toCLDeviceID(engine->devices, cl_devices);

	OCLCodeObject * code_obj = nullptr;
	//std::string build_opt = GetBuildOption();

	cl_context ctx = engine->context;
	int num_devices = 1;

	cl_int *per_status = new cl_int[num_devices];
	size_t * bin_len = new size_t[num_devices];
	for(int i=0;i<num_devices;i++)
		bin_len[i] = (size_t)bytes;
	unsigned char ** bin_data = new unsigned char *[num_devices];
	for(int i=0;i<num_devices;i++)
		bin_data[i] = (unsigned char*)content;

	cl_int status;
	cl_device_id dev_id = (cl_device_id)dev->object();
	program = clCreateProgramWithBinary(ctx, num_devices, &dev_id, bin_len, 
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
	status = clBuildProgram(program, num_devices, &dev_id, NULL, NULL, NULL);

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

std::string OCLASMCompiler::GetBuildOption(){
	return "-x assembler -target amdgcn--amdhsa -mcpu=gfx900";
}

CodeObject * OCLASMCompiler::operator()(const unsigned char * content, int bytes, DeviceBase * dev){
	// hard coded to use opencl clang in rocm install folder 
	std::string compiler = "/opt/rocm/opencl/bin/x86_64/clang";
	std::string tmp_dir =  GenerateTmpDir();
	std::string src_file = tmp_dir + "/src.s";
	std::string target_file = tmp_dir + "/target.co";

	// First, dump source to tmp directory

	//std::cout<<"  src/obj in "<<tmp_dir<<std::endl;

	// write to src file
	{
		std::ofstream ofs(src_file.c_str(), std::ios::binary);
		ofs.write((const char*)content, bytes);
		ofs.flush();
	}

	std::string cmd;
	cmd = compiler + " " + GetBuildOption() + " -o " + target_file + " " + src_file;

	if(!ExecuteCmdSync(cmd.c_str())){
		std::cerr<<"ERROR: compile fail for \""<<cmd<<"\"";
		return nullptr;
	}

	// Second, load bin file and use OCL api to build
	unsigned char * bin_content;
	int bin_bytes;
	if(GetFileContent(target_file.c_str(), &bin_content, &bin_bytes) != E_ReturnState::SUCCESS){
		std::cerr<<"ERROR: fail to get object file "<<target_file<<std::endl;
		return nullptr;
	}
	cl_context ctx = engine->context;
	cl_int status;
	cl_device_id dev_id = (cl_device_id)dev->object();
	const unsigned char * cl_bin[1] = {bin_content};
	size_t cl_bytes = bin_bytes;
	cl_program program = clCreateProgramWithBinary(ctx, 1, &dev_id , &cl_bytes, 
			cl_bin, NULL, &status);

	if(status != CL_SUCCESS){
		std::cerr<<"clCreateProgramWithBinary faild with binary, status:"<<status<<std::endl;
		delete [] bin_content;
		return nullptr;
	}
	delete [] bin_content;
	// ignore build option for from binary source
	status = clBuildProgram(program, 1, &dev_id, NULL, NULL, NULL);
	if(status != CL_SUCCESS){
		std::cerr<<"clBuildProgram faild, status:"<<status<<std::endl;
		DumpCLBuildLog(program, dev_id);
		clReleaseProgram(program);
		return nullptr;
	}

	CodeObject * code_obj = new OCLCodeObject(program);
	return (CodeObject*)code_obj;
}

std::string OCLCCompiler::GetBuildOption(){
	return "";
}
CodeObject * OCLCCompiler::operator()(const unsigned char * content, int bytes, DeviceBase * dev){
	cl_context ctx = engine->context;
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