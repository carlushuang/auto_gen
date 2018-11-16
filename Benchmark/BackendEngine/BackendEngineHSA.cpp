#include "BackendEngine.h"
#include <iostream>
#include <string.h>
#include <stdio.h>

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

class HSADevice : public DeviceBase{
	HSADevice(void * obj, BackendEngineHSA * re):DeviceBase(obj), engine(re){}
	~HSADevice(){}
	virtual StreamBase * CreateStream(bool is_async=false){
	}
	virtual void GetDeviceInfo(DeviceInfo * dev_info){
	}
};

BackendEngineHSA BackendEngineHSA::INSTANCE;



static hsa_status_t get_agent_callback(hsa_agent_t agent, void *data){
	if (!data)
		return HSA_STATUS_ERROR_INVALID_ARGUMENT;

	std::vector<hsa_agent_t> * gpu_agents = (std::vector<hsa_agent_t> *)data;
	hsa_device_type_t hsa_device_type;
	hsa_status_t hsa_error_code = hsa_agent_get_info(agent, HSA_AGENT_INFO_DEVICE, &hsa_device_type);
	if (hsa_error_code != HSA_STATUS_SUCCESS)
		return hsa_error_code;

	if (hsa_device_type == HSA_DEVICE_TYPE_GPU) {
		gpu_agents->push_back(agent);
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

	hsa_backend* b = static_cast<hsa_backend*>(data);

	if (flags & HSA_REGION_GLOBAL_FLAG_FINE_GRAINED) {
		b->system_region_ = region;
	}

	if (flags & HSA_REGION_GLOBAL_FLAG_COARSE_GRAINED) {
		if(host_accessible_region){
			b->local_region_ = region;
		}else{
			b->gpu_local_region_ = region;
		}
	}

	if (flags & HSA_REGION_GLOBAL_FLAG_KERNARG) {
		b->kernarg_region_ = region;
	}

	return HSA_STATUS_SUCCESS;
}

E_ReturnState BackendEngineHSA::Init(){
	hsa_status_t status;
    status = hsa_init();
    HSA_ENFORCE("hsa_init", status);

    // Find GPU
	std::vector<hsa_agent_t> gpu_agents;
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

    char agent_name[64];

    status = hsa_agent_get_info(agent_, HSA_AGENT_INFO_NAME, agent_name);
    HSA_ENFORCE("hsa_agent_get_info(HSA_AGENT_INFO_NAME)", status);

    std::cout << "Using agent: " << agent_name << std::endl;

    status = hsa_agent_get_info(agent_, HSA_AGENT_INFO_QUEUE_MAX_SIZE, &queue_size_);
    HSA_ENFORCE("hsa_agent_get_info(HSA_AGENT_INFO_QUEUE_MAX_SIZE", status);

    // Create a queue in the kernel agent. The queue can hold 4 packets, and has no callback or service queue associated with it
    status = hsa_queue_create(agent_, queue_size_, HSA_QUEUE_TYPE_MULTI, NULL, NULL, UINT32_MAX, UINT32_MAX, &queue_);
    HSA_ENFORCE("hsa_queue_create", status);

    status = hsa_signal_create(1, 0, NULL, &signal_);
    HSA_ENFORCE("hsa_signal_create", status);

    status = hsa_agent_iterate_regions(agent_, get_region_callback, this);
    HSA_ENFORCE("hsa_agent_iterate_regions", status);

    HSA_ENFORCE_PTR("Failed to find kernarg memory region", kernarg_region_.handle)

    inited = 1;
    return 0;
}