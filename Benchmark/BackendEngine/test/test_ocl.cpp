#include <BackendEngine.h>
#include <iostream>
#include <assert.h>
#include <fstream>

#include <random>
#include <math.h>


const int ARRAY_SIZE = 1000;

const int GROUP_X = 64;
const int GRID_X = 10;

static void random_vec(float * vec, int num){
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<float> dist(0.01, 1.0);
	for(int i=0;i<num;i++){
		vec[i] = dist(e2);
	}
}

//#define ENGINE() BackendEngine::Get("OpenCL")
#define ENGINE() BackendEngine::Get("HSA")

int main(){
	//BackendEngineBase * engine = BackendEngine::Get("OpenCL");
	//assert(ENGINE());

	DeviceBase * dev = ENGINE()->GetDevice(0);
	{
		DeviceInfo dev_info;
		dev->GetDeviceInfo(&dev_info);
		DumpDeviceInfo(&dev_info);
	}
	StreamBase* stream = dev->CreateStream();

	E_ReturnState rtn;
	unsigned char * bin_content;
	int bin_size;
#if 0
	rtn = GetFileContent("vector_add.bin", &bin_content, &bin_size);
	assert(rtn == E_ReturnState::SUCCESS);

	auto * compiler = new OCLBinaryCompiler(ENGINE());
#endif
#if 0
	rtn = GetFileContent("vector_add.cl", &bin_content, &bin_size);
	assert(rtn == E_ReturnState::SUCCESS);

	auto * compiler = new OCLCCompiler(ENGINE());
#endif
#if 0
	rtn = GetFileContent("vector_add.s", &bin_content, &bin_size);
	assert(rtn == E_ReturnState::SUCCESS);

	auto * compiler = new OCLASMCompiler(ENGINE());
#endif
	rtn = GetFileContent("vector_add.bin", &bin_content, &bin_size);
	assert(rtn == E_ReturnState::SUCCESS);

	auto * compiler = new HSABinaryCompiler(ENGINE());


	CodeObject * code_obj = (*compiler)(bin_content, bin_size, dev);
	assert(code_obj);

	delete compiler;
	delete [] bin_content;

	// dump code object to file
	std::ofstream ofs("vector_add_dump.bin", std::ios::binary);
	code_obj->Serialize(ofs);

	KernelObject * kernel_obj = code_obj->CreateKernelObject("vector_add");
	assert(kernel_obj);

	delete code_obj;

	// prepare kernel arg
	void * d_mem_a = ENGINE()->AllocDeviceMem(ARRAY_SIZE * sizeof(float), dev);
	void * d_mem_b = ENGINE()->AllocDeviceMem(ARRAY_SIZE * sizeof(float), dev);
	void * d_mem_c = ENGINE()->AllocDeviceMem(ARRAY_SIZE * sizeof(float), dev);

	float * h_a = new float[ARRAY_SIZE];
	float * h_b = new float[ARRAY_SIZE];
	float * h_c = new float[ARRAY_SIZE];
	float * h_c_dev = new float[ARRAY_SIZE];
	random_vec(h_a, ARRAY_SIZE);
	random_vec(h_b, ARRAY_SIZE);
	{
		// calculate host
		for(int i=0;i<ARRAY_SIZE;i++){
			h_c[i] = h_a[i] + h_b[i];
		}
	}

	rtn = ENGINE()->Memcpy(d_mem_a, h_a, ARRAY_SIZE * sizeof(float), MEMCPY_HOST_TO_DEV, stream);
	assert(rtn == E_ReturnState::SUCCESS);
	rtn = ENGINE()->Memcpy(d_mem_b, h_b, ARRAY_SIZE * sizeof(float), MEMCPY_HOST_TO_DEV, stream);
	assert(rtn == E_ReturnState::SUCCESS);

	kernel_obj->SetKernelArg(&d_mem_a);
	kernel_obj->SetKernelArg(&d_mem_b);
	kernel_obj->SetKernelArg(&d_mem_c);
	kernel_obj->SetKernelArg(&ARRAY_SIZE);


	DispatchParam dispatch_param;
	dispatch_param.local_size[0] = GROUP_X;
	dispatch_param.local_size[1] = 0;
	dispatch_param.local_size[2] = 0;
	dispatch_param.global_size[0] = GROUP_X * GRID_X;
	dispatch_param.global_size[1] = 0;
	dispatch_param.global_size[2] = 0;

	dispatch_param.kernel_obj = kernel_obj;

	stream->SetupDispatch(&dispatch_param);
	stream->Launch(1);
	stream->Wait();

	rtn = ENGINE()->Memcpy(h_c_dev, d_mem_c, ARRAY_SIZE * sizeof(float), MEMCPY_DEV_TO_HOST, stream);
	assert(rtn == E_ReturnState::SUCCESS);

	{
		bool is_valid = true;
		// valid device calculation
		for(int i=0;i<ARRAY_SIZE;i++){
			float delta = fabsf(h_c_dev[i] - h_c[i]);
			if(delta > 0.0001){
				std::cerr<<"calculation error, host:"<<h_c[i]<<", dev:"<<h_c_dev[i]<<std::endl;
				is_valid = false;
			}
		}
		std::cout<<"Compute valid:"<< (is_valid?"valid":"false") <<std::endl;
	}

	ENGINE()->Free(d_mem_a);
	ENGINE()->Free(d_mem_b);
	ENGINE()->Free(d_mem_c);

	delete [] h_a;
	delete [] h_b;
	delete [] h_c;
	delete [] h_c_dev;

	delete kernel_obj;

	delete stream;
	return 0;
}