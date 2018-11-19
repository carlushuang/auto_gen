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

class HSAStream : public StreamBase{
	HSAStream(void * stream_obj, DeviceBase * dev_, bool is_async_) :
			StreamBase(stream_obj, dev_, is_async_)
	{}
	~HSAStream(){

	}
	virtual E_ReturnState SetupDispatch(DispatchParam * dispatch_param){
		// Request a packet ID from the queue. Since no packets have been enqueued yet, the expected ID is zero
		packet_index_ = hsa_queue_add_write_index_relaxed(queue_, 1);
		const uint32_t queue_mask = queue_->size - 1;
		aql_ = (hsa_kernel_dispatch_packet_t*) (hsa_kernel_dispatch_packet_t*)(queue_->base_address) + (packet_index_ & queue_mask);
		const size_t aql_header_size = 4;
		memset((uint8_t*)aql_ + aql_header_size, 0, sizeof(*aql_) - aql_header_size);
		// initialize_packet
		aql_->completion_signal = signal_;
		aql_->workgroup_size_x = 1;
		aql_->workgroup_size_y = 1;
		aql_->workgroup_size_z = 1;
		aql_->grid_size_x = 1;
		aql_->grid_size_y = 1;
		aql_->grid_size_z = 1;
		aql_->group_segment_size = 0;
		aql_->private_segment_size = 0;
		// executable
		if (0 != load_bin_from_file(d_param->code_file_name.c_str()))
			return -1;
		hsa_status_t status = hsa_executable_create(HSA_PROFILE_FULL, HSA_EXECUTABLE_STATE_UNFROZEN,
									NULL, &executable_);
		HSA_ENFORCE("hsa_executable_create", status);
		// Load code object
		status = hsa_executable_load_code_object(executable_, agent_, code_object_, NULL);
		HSA_ENFORCE("hsa_executable_load_code_object", status);
		// Freeze executable
		status = hsa_executable_freeze(executable_, NULL);
		HSA_ENFORCE("hsa_executable_freeze", status);

		// Get symbol handle
		hsa_executable_symbol_t kernel_symbol;
		status = hsa_executable_get_symbol(executable_, NULL, d_param->kernel_symbol.c_str(), agent_,
											0, &kernel_symbol);
		HSA_ENFORCE("hsa_executable_get_symbol", status);

		// Get code handle
		uint64_t code_handle;
		status = hsa_executable_symbol_get_info(kernel_symbol,
												HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_OBJECT,
												&code_handle);
		HSA_ENFORCE("hsa_executable_symbol_get_info", status);
		status = hsa_executable_symbol_get_info(kernel_symbol,
						HSA_EXECUTABLE_SYMBOL_INFO_KERNEL_GROUP_SEGMENT_SIZE,
						&group_static_size_);
		HSA_ENFORCE("hsa_executable_symbol_get_info", status);

		aql_->kernel_object = code_handle;

		// kernel args
		void *kernarg;
		status = hsa_memory_allocate(kernarg_region_, d_param->kernel_arg_size, &kernarg);
		HSA_ENFORCE("hsa_memory_allocate", status);
		aql_->kernarg_address = kernarg;
		size_t kernarg_offset = 0;
		for(auto & ptr : d_param->kernel_arg_list){
			feed_kernarg(ptr.get(), kernarg_offset);
		}
		aql_->workgroup_size_x = d_param->local_size[0];
		aql_->workgroup_size_y = d_param->local_size[1];
		aql_->workgroup_size_z = d_param->local_size[2];

		aql_->grid_size_x = d_param->global_size[0];
		aql_->grid_size_y = d_param->global_size[1];
		aql_->grid_size_z = d_param->global_size[2];
	}
	virtual E_ReturnState Launch(int iter){
	}
	virtual E_ReturnState Wait() {
	}
};

class HSADevice : public DeviceBase{
public:
	HSADevice(void * obj, BackendEngineHSA * re):DeviceBase(obj), engine(re){}
	~HSADevice(){
		hsa_agent_t * agent = (hsa_agent_t*)object();
		delete agent;
	}
	virtual StreamBase * CreateStream(bool is_async=false){
		hsa_queue_t * queue;
		hsa_agent_t * agent = (hsa_agent_t*)object();
		// TODO: HSA may limit to signle queue if 
		hsa_queue_type_t q_type;
		unsigned int queue_size;
		hsa_status_t status;
		status = hsa_agent_get_info(*agent, HSA_AGENT_INFO_QUEUE_TYPE, &q_type);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to query HSA_AGENT_INFO_QUEUE_TYPE from this agent."<<std:endl;
			return nullptr;
		}
		status = hsa_agent_get_info(*agent, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size);
		if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to query HSA_AGENT_INFO_QUEUE_MAX_SIZE from this agent."<<std:endl;
			return nullptr;
		}
		status = hsa_queue_create(*agent, queue_size, q_type, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue);
    	if(status != HSA_STATUS_SUCCESS){
			std::cerr<<"ERROR: fail to hsa_queue_create"<<std:endl;
			return nullptr;
		}

		HSAStream * stream = new HSAStream(queue, this, is_async);

	}
	virtual void GetDeviceInfo(DeviceInfo * dev_info){
		hsa_agent_t agent = (hsa_agent_t)object();
		char v_char[64];
		unsigned int v_uint;
		HSA_DEV_INFO(agent, HSA_AGENT_INFO_NAME, v_char );
		dev_info->DeviceName = v_char;

		HSA_DEV_INFO(agent, HSA_AGENT_INFO_COMPUTE_UNIT_COUNT, &v_uint );
		dev_info->ComputeUnitNum = v_uint;

		HSA_DEV_INFO(agent, HSA_AGENT_INFO_MAX_CLOCK_FREQUENCY, &v_uint );
		dev_info->CoreFreq = v_uint;	//Hz

		dev_info->GlobalMemSize = 0;	// HSA have no info for memory size?
		dev_info->ProcessingElementNum = dev_info->ComputeUnitNum * GPU_SIMD_NUM_PER_CU * GPU_ALU_NUM_PER_SIMD;
		dev_info->Fp32Flops = dev_info->ProcessingElementNum * dev_info->CoreFreq * 2;
	}

	// http://www.hsafoundation.com/html/Content/Runtime/Topics/02_Core/memory.htm
    hsa_region_t system_region;
    hsa_region_t kernarg_region;
    hsa_region_t local_region;
    hsa_region_t gpu_local_region;
};

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
		ptr_agent->handle = agent.handle;	// carefull handle opaque struct
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
		std::unique_ptr<DeviceBase> dev(new HSADevice(gpu_agents[i], this));
		devices.push_back(std::move(dev));
	}

	for(int i=0;i<devices.size();i++){
		HSADevice * dev = devices[i].get();
		hsa_agent_t * agent = (hsa_agent_t *)dev->object();
		status = hsa_agent_iterate_regions(*agent, get_region_callback, dev);
		HSA_ENFORCE("hsa_agent_iterate_regions", status);
	}

    inited = 1;
    return E_ReturnState::SUCCESS;
}

E_ReturnState BackendEngineHSA::Destroy(){

}
int GetDeviceNum() const{

}
DeviceBase * GetDevice(int index){

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