#include "BackendEngine.h"
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#define		GPU_CORE_FREQ_HZ		(1.5e9)
#define		GPU_SE_NUM				(4)
#define		GPU_CU_NUM_PER_SE		(16)
#define		GPU_SIMD_NUM_PER_CU		(4)
#define		GPU_ALU_NUM_PER_SIMD	(16)
#define		GPU_ALU_NUM				(GPU_ALU_NUM_PER_SIMD*GPU_SIMD_NUM_PER_CU*GPU_CU_NUM_PER_SE*GPU_SE_NUM)
#define		GPU_CALCULATION			(GPU_ALU_NUM * GPU_CORE_FREQ_HZ)

#define HSA_ENFORCE(msg, rtn) \
            if(rtn != HSA_STATUS_SUCCESS) {\
                const char * err; \
                hsa_status_string(rtn, &err); \
                std::cerr<<"ERROR:"<<msg<<", rtn:"<<rtn<<", "<<err<<std::endl;\
                return E_ReturnState::FAIL; \
            }

#define HSA_ENFORCE_PTR(msg, ptr) \
            if(!ptr) {\
                std::cerr<<"ERROR:"<<msg<<std::endl;\
                return E_ReturnState::FAIL; \
            }


#define HSA_DEV_INFO(agent, info, store) \
	do{					\
		hsa_status_t status = hsa_agent_get_info(agent, info, store);	\
		if(status != HSA_STATUS_SUCCESS){			\
			std::cerr<<"ERROR: hsa_agent_get_info("#info") fail"<<std::endl; \
			return;	\
		}	\
	}while(0)

// store more info
struct HSAKernelObject: public KernelObject{
public:
	HSAKernelObject(void * obj):KernelObject(obj){}
	~HSAKernelObject(){}

	// get this info from kernel object and set to aql
	uint32_t group_segment_size;
	uint32_t private_segment_size;
};

class HSADevice : public DeviceBase{
public:
	HSADevice(void * obj, BackendEngineHSA * re):DeviceBase(obj), engine(re){}
	~HSADevice(){
		hsa_agent_t * agent = (hsa_agent_t*)object();
		delete agent;
	}
	virtual StreamBase * CreateStream(bool is_async=false);
	virtual void GetDeviceInfo(DeviceInfo * dev_info);

	int index;
	BackendEngineHSA * engine;

	// http://www.hsafoundation.com/html/Content/Runtime/Topics/02_Core/memory.htm
    hsa_region_t system_region;
    hsa_region_t kernarg_region;
    hsa_region_t local_region;
    hsa_region_t gpu_local_region;
};

struct HSACodeObject : public CodeObject{
	HSACodeObject(void * obj):CodeObject(obj){}
	~HSACodeObject(){
		hsa_code_object_t * code_obj = (hsa_code_object_t *)object();
		hsa_code_object_destroy(*code_obj);
		delete code_obj;

		hsa_executable_destroy(executable);
	}
public:
	virtual E_ReturnState Serialize(std::ostream & os ){}
	virtual KernelObject * CreateKernelObject(const char * kernel_name){
		hsa_code_object_t * code_obj = (hsa_code_object_t *)object();
		hsa_agent_t * agent = (hsa_agent_t*) dev->object();

		hsa_status_t status = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN,
                                 NULL, &executable);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_create, status:"<<status<<std::endl;
			return nullptr;
		}

		status = hsa_executable_load_code_object(executable, *agent, *code_obj, NULL);
    	if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_load_code_object, status:"<<status<<std::endl;
			return nullptr;
		}

		status = hsa_executable_freeze(executable, NULL);
    	if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_freeze, status:"<<status<<std::endl;
			return nullptr;
		}
		hsa_executable_symbol_t kernel_symbol;
    	status = hsa_executable_get_symbol(executable, NULL, kernel_name, *agent, 0, &kernel_symbol);
    	if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_get_symbol, status:"<<status<<std::endl;
			return nullptr;
		}

		uint64_t code_handle;
		status = hsa_executable_symbol_get_info(kernel_symbol,
												HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT,
												&code_handle);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_symbol_get_info(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT), status:"
				<<status<<std::endl;
			return nullptr;
		}
		uint32_t group_segment_size;
		uint32_t private_segment_size;
		status = hsa_executable_symbol_get_info(kernel_symbol,
                    HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE,
                    &group_segment_size);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_symbol_get_info(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE), status:"
				<<status<<std::endl;
			return nullptr;
		}
		status = hsa_executable_symbol_get_info(kernel_symbol,
                    HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_PRIVATE_SEGMENT_SIZE,
                    &private_segment_size);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_executable_symbol_get_info(HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE), status:"
				<<status<<std::endl;
			return nullptr;
		}
		HSAKernelObject * hsa_kernel_obj = new HSAKernelObject(reinterpret_cast<void*>(code_handle));
		hsa_kernel_obj->group_segment_size = group_segment_size;
		hsa_kernel_obj->private_segment_size = private_segment_size;

		return (KernelObject*)hsa_kernel_obj;
	}

	DeviceBase * dev;
	hsa_executable_t executable;
};


static void aligned_copy_kernarg(void * kernarg, const void * ptr, size_t size, size_t align, size_t & offset){
    //assert((align & (align - 1)) == 0);
    offset = ((offset + align - 1) / align) * align;
	if(kernarg && ptr)
    	memcpy( (char*)kernarg + offset, ptr, size);
    offset += size;
}

static size_t GetKernArgSize(KernelObject * kernel_obj){
	size_t bytes = 0;
	for(auto & k_arg : kernel_obj->kernel_args)
		aligned_copy_kernarg(nullptr, nullptr, k_arg->bytes, k_arg->bytes, bytes);
	return bytes;
}

class HSAStream : public StreamBase{
public:
	HSAStream(void * stream_obj, DeviceBase * dev_, bool is_async_) :
			StreamBase(stream_obj, dev_, is_async_)
	{}
	~HSAStream(){
		hsa_queue_t* queue = (hsa_queue_t*)object();
		hsa_queue_destroy(queue);
		hsa_signal_destroy(d_signal);
	}
	virtual E_ReturnState SetupDispatch(DispatchParam * dispatch_param){
		HSAKernelObject * kernel_obj = (HSAKernelObject*)dispatch_param->kernel_obj;
		HSADevice * device = (HSADevice*)this->dev;
		hsa_queue_t* queue = (hsa_queue_t*)object();
		hsa_status_t status = hsa_signal_create(1, 0, NULL, &d_signal);
    	HSA_ENFORCE("hsa_signal_create", status);
		// Request a packet ID from the queue. Since no packets have been enqueued yet, the expected ID is zero
		packet_index = hsa_queue_add_write_index_relaxed(queue, 1);
		const uint32_t queue_mask = queue->size - 1;

		aql = (hsa_kernel_dispatch_packet_t*)(queue->base_address) + (packet_index & queue_mask);
		const size_t aql_header_size = 4;
		memset((uint8_t*)aql + aql_header_size, 0, sizeof(*aql) - aql_header_size);
		// initialize_packet
		aql->completion_signal = d_signal;
		//aql->group_segment_size = 0;
		//aql->private_segment_size = 0;

		aql->kernel_object = reinterpret_cast<uint64_t> (kernel_obj->object());

		// kernel args
		void *kernarg;
		size_t kernel_arg_size = GetKernArgSize(kernel_obj);
		status = hsa_memory_allocate(device->kernarg_region, kernel_arg_size, &kernarg);
		HSA_ENFORCE("hsa_memory_allocate", status);
		aql->kernarg_address = kernarg;
		size_t kernarg_offset = 0;
		for(auto & ptr : kernel_obj->kernel_args){
			aligned_copy_kernarg(kernarg, ptr->content, ptr->bytes, ptr->bytes, kernarg_offset);
		}
		aql->workgroup_size_x = dispatch_param->local_size[0];
		aql->workgroup_size_y = dispatch_param->local_size[1];
		aql->workgroup_size_z = dispatch_param->local_size[2];

		aql->grid_size_x = dispatch_param->global_size[0];
		aql->grid_size_y = dispatch_param->global_size[1];
		aql->grid_size_z = dispatch_param->global_size[2];

		dim = 1;
		if (aql->grid_size_y > 1)
			dim = 2;
		if (aql->grid_size_z > 1)
			dim = 3;
		
		// hint for the size
		//aql->group_segment_size = UINT32_MAX;
		aql->group_segment_size = kernel_obj->group_segment_size;
		aql->private_segment_size = kernel_obj->private_segment_size;

		header =
			(HSA_PACKET_TYPE_KERNEL_DISPATCH << HSA_PACKET_HEADER_TYPE) |
			(1 << HSA_PACKET_HEADER_BARRIER) |
			(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_ACQUIRE_FENCE_SCOPE) |
			(HSA_FENCE_SCOPE_SYSTEM << HSA_PACKET_HEADER_RELEASE_FENCE_SCOPE);
		header &= 0xffff;
		uint16_t setup = dim << HSA_KERNEL_DISPATCH_PACKET_SETUP_DIMENSIONS;
    	header = header | (setup << 16);

		return E_ReturnState::SUCCESS;
	}
	virtual E_ReturnState Launch(int iter){
		hsa_queue_t* queue = (hsa_queue_t*)object();
		for(int i=0;i<iter;i++){
			__atomic_store_n((uint32_t*)aql, header, __ATOMIC_RELEASE);
    		hsa_signal_store_relaxed(queue->doorbell_signal, packet_index);
		}
	}
	virtual E_ReturnState Wait() {
		hsa_signal_value_t result;
    	result = hsa_signal_wait_acquire(d_signal, HSA_SIGNAL_CONDITION_EQ, 0, UINT64_MAX, HSA_WAIT_STATE_ACTIVE);
		E_ReturnState::SUCCESS;
	}

	hsa_kernel_dispatch_packet_t* aql;
	hsa_signal_t d_signal;
	uint64_t packet_index;
	uint16_t dim;
	uint32_t header;
};

StreamBase * HSADevice::CreateStream(bool is_async){
	hsa_queue_t * queue;
	hsa_agent_t * agent = (hsa_agent_t*)object();
	// TODO: HSA may limit to signle queue if 
	hsa_queue_type_t q_type;
	unsigned int queue_size;
	hsa_status_t status;
	status = hsa_agent_get_info(*agent, HSA_AGENT_INFO_QUEUE_TYPE, &q_type);
	if(status != HSA_STATUS_SUCCESS){
		std::cerr<<"ERROR: fail to query HSA_AGENT_INFO_QUEUE_TYPE from this agent."<<std::endl;
		return nullptr;
	}
	status = hsa_agent_get_info(*agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
	if(status != HSA_STATUS_SUCCESS){
		std::cerr<<"ERROR: fail to query HSA_AGENT_INFO_QUEUE_MAX_SIZE from this agent."<<std::endl;
		return nullptr;
	}
	status = hsa_queue_create(*agent, queue_size, q_type, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
	if(status != HSA_STATUS_SUCCESS){
		std::cerr<<"ERROR: fail to hsa_queue_create"<<std::endl;
		return nullptr;
	}

	HSAStream * stream = new HSAStream(queue, this, is_async);
	return stream;
}

void HSADevice::GetDeviceInfo(DeviceInfo * dev_info){
	hsa_agent_t * agent = (hsa_agent_t*)object();
	char v_char[64];
	unsigned int v_uint;

	dev_info->DeviceIdx = this->index;

	HSA_DEV_INFO(*agent, HSA_AGENT_INFO_NAME, v_char );
	dev_info->DeviceName = v_char;

	HSA_DEV_INFO(*agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_COMPUTE_UNIT_COUNT, &v_uint );
	dev_info->ComputeUnitNum = v_uint;

	HSA_DEV_INFO(*agent, (hsa_agent_info_t)HSA_AMD_AGENT_INFO_MAX_CLOCK_FREQUENCY, &v_uint );
	dev_info->CoreFreq = v_uint * 1e6;	//MHz -> Hz

	dev_info->GlobalMemSize = 0;	// HSA have no info for memory size?
	dev_info->ProcessingElementNum = dev_info->ComputeUnitNum * GPU_SIMD_NUM_PER_CU * GPU_ALU_NUM_PER_SIMD;
	dev_info->Fp32Flops = dev_info->ProcessingElementNum * dev_info->CoreFreq * 2;
}


BackendEngineHSA BackendEngineHSA::INSTANCE;

static hsa_status_t get_agent_callback(hsa_agent_t agent, void *data){
	if (!data)
		return HSA_STATUS_ERROR_INVALID_ARGUMENT;

	std::vector<hsa_agent_t*> * gpu_agents = (std::vector<hsa_agent_t*> *)data;
	hsa_device_type_t hsa_device_type;
	hsa_status_t hsa_error_code = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &hsa_device_type);
	if (hsa_error_code != HSA_STATUS_SUCCESS)
		return hsa_error_code;

	if (hsa_device_type == HSA_DEVICE_TYPE_GPU) {
		hsa_agent_t * ptr_agent = new hsa_agent_t;
		memcpy(ptr_agent, &agent, sizeof(hsa_agent_t));
		gpu_agents->push_back(ptr_agent);
	}
	if (hsa_device_type == HSA_DEVICE_TYPE_CPU) {
		//
	}

	return HSA_STATUS_SUCCESS;
}

static hsa_status_t get_region_callback(hsa_region_t region, void* data)
{
	hsa_region_segment_t segment_id;
	hsa_region_get_info(region, HSA_REGION_INFO_SEGMENT, &segment_id);

	if (segment_id != HSA_REGION_SEGMENT_GLOBAL) {
		return HSA_STATUS_SUCCESS;
	}

	hsa_region_global_flag_t flags;
	bool host_accessible_region = false;
	hsa_region_get_info(region, HSA_REGION_INFO_GLOBAL_FLAGS, &flags);
	hsa_region_get_info(region, (hsa_region_info_t)HSA_AMD_REGION_INFO_HOST_ACCESSIBLE, &host_accessible_region);

	HSADevice * dev = static_cast<HSADevice*>(data);

	if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED) {
		dev->system_region = region;
	}

	if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED) {
		if(host_accessible_region){
			dev->local_region = region;
		}else{
			dev->gpu_local_region = region;
		}
	}

	if (flags & HSA_REGION_GLOBAL_FLAG_KERNARG) {
		dev->kernarg_region = region;
	}

	return HSA_STATUS_SUCCESS;
}

E_ReturnState BackendEngineHSA::Init(){
	hsa_status_t status;

	// Ignore multiple init
	if(Inited())
		return E_ReturnState::SUCCESS;

    status = hsa_init();
    HSA_ENFORCE("hsa_init", status);

    // Find GPU
	std::vector<hsa_agent_t*> gpu_agents;
    status = hsa_iterate_agents(get_agent_callback, &gpu_agents);
    HSA_ENFORCE("hsa_iterate_agents", status);
	if(gpu_agents.size() == 0){
		std::cerr<<"No device under HSA runtime, init fail"<<std::endl;
		return E_ReturnState::FAIL;
	}

	for(int i=0;i<gpu_agents.size();i++){
		HSADevice * dev = new HSADevice(gpu_agents[i], this);
		dev->index = i;
		std::unique_ptr<DeviceBase> p_dev(dev);
		devices.push_back(std::move(p_dev));
	}

	for(int i=0;i<devices.size();i++){
		HSADevice * dev = (HSADevice * ) devices[i].get();
		hsa_agent_t * agent = (hsa_agent_t *)dev->object();
		status = hsa_agent_iterate_regions(*agent, get_region_callback, dev);
		HSA_ENFORCE("hsa_agent_iterate_regions", status);
	}

    inited = 1;
    return E_ReturnState::SUCCESS;
}

E_ReturnState BackendEngineHSA::Destroy(){
	
}
int BackendEngineHSA::GetDeviceNum() const{
	return devices.size();
}
DeviceBase * BackendEngineHSA::GetDevice(int index){
	if(index > (devices.size()-1))
		return nullptr;
	return devices[index].get();
}
void * BackendEngineHSA::AllocDeviceMem(int bytes, DeviceBase * dev){
	if(!dev){
		std::cerr<<"current HSA backend need Device as input param"<<std::endl;
		return nullptr;
	}

	// TODO: we use system_region here for device memory, as fine-grind region
	hsa_region_t region = ((HSADevice*)dev)->system_region;
	void * ptr;
		
	hsa_status_t status = hsa_memory_allocate(region, bytes, (void **)&ptr);
    if (status != HSA_STATUS_SUCCESS){
        std::cerr<<"hsa_memory_allocate failed, "<< status <<std::endl;
        return nullptr;
    }
    return ptr;
}
void * BackendEngineHSA::AllocPinnedMem(int bytes, DeviceBase * dev){

}

E_ReturnState BackendEngineHSA::Memcpy(void * dst, void * src, int bytes, enum MEMCPY_TYPE memcpy_type, StreamBase * stream){
	// HSA unified memory model allow use to access
	hsa_status_t status;
	status = hsa_memory_copy(dst, src, bytes);
	HSA_ENFORCE("fail to hsa_memory_copy host to dev", status);
#if 0
	switch(memcpy_type){
		case MEMCPY_HOST_TO_DEV:
			status = hsa_memory_copy(dst, src, bytes);
			HSA_ENFORCE("fail to hsa_memory_copy host to dev", status);
		break;
		case MEMCPY_DEV_TO_HOST:
			status = hsa_memory_copy(dst, src, bytes);
			HSA_ENFORCE("fail to hsa_memory_copy host to dev", status);
		break;
		case MEMCPY_DEV_TO_DEV:
		break;
		default:
		break;
	}
#endif
	return E_ReturnState::SUCCESS;
}

void BackendEngineHSA::Free(void * mem){
	if(mem)
		hsa_memory_free(mem);
}

std::string HSABinaryCompiler::GetBuildOption(){
	return "";
}

CodeObject * HSABinaryCompiler::operator()(const unsigned char * content, int bytes, DeviceBase * dev){
	hsa_code_object_t code_object;
	hsa_status_t status = hsa_code_object_deserialize((void*)content, bytes, NULL, &code_object);
	if(status != HSA_STATUS_SUCCESS){
		std::cerr<<"ERROR: fail to hsa_code_object_deserialize, status:"<<status<<std::endl;
		return nullptr;
	}

	hsa_code_object_t * p_code_obj = new hsa_code_object_t;
	memcpy(p_code_obj, &code_object, sizeof(hsa_code_object_t));

	HSACodeObject * hsa_code_obj = new HSACodeObject(p_code_obj);
	hsa_code_obj->dev = dev;
	return hsa_code_obj;
}