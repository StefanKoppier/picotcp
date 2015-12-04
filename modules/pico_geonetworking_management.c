#include "pico_geonetworking_management.h"

struct pico_gn_management_interface pico_gn_mgmt_interface = {
    .get_time = NULL,
    .get_position = NULL,
};

void pico_gn_settings_set_implementation(enum pico_gn_setting_name name, void *value)
{
    size_t size = 0;
    
    switch (name)
    {
    case LOCAL_GN_ADDRESS:
        size = sizeof(struct pico_gn_address);
        break;
    case LOCAL_ADDR_CONF_METHOD:
        size = sizeof(enum pico_gn_address_conf_method);
        break;
    case PROTOCOL_VERSION:
        size = sizeof(uint8_t);
        break;
    case STATION_TYPE:
        size = sizeof(enum pico_gn_station_type);
        break;
    case IS_MOBILE:
        size = sizeof(uint8_t);
        break;
    case INTERFACE_TYPE:
        size = sizeof(enum pico_gn_interface_type);
        break;
    case MIN_UPDATE_FREQUENCY_LPV:
        size = sizeof(uint16_t);
        break;
    case PAI_INTERVAL:
        size = sizeof(uint8_t);
        break;
    case MAX_SDU_SIZE:
        size = sizeof(uint16_t);
        break;
    case MAX_GN_HEADER_SIZE:
        size = sizeof(uint16_t);
        break;
    case LIFETIME_LOCTE:
        size = sizeof(uint16_t);
        break;
    case SECURITY:
        size = sizeof(enum pico_gn_security);
        break;
    case SN_DECAP_RESULT_HANDLING:
        size = sizeof(enum pico_gn_sn_decap_result_handling);
        break;
    case LOCATION_SERVICE_MAX_RETRANSMIT:
        size = sizeof(uint8_t);
        break;
    case LOCATION_SERVICE_RETRANSMIT_TIMER:
        size = sizeof(uint16_t);
        break;
    case LOCATION_SERVICE_PACKET_BUFFER_SIZE:
        size = sizeof(uint16_t);
        break;
    case BEACON_SERVICE_RETRANSMIT_TIMER:
        size = sizeof(uint16_t);
        break;
    case BEACON_SERVICE_MAX_JITTER:
        size = sizeof(uint16_t);
        break;
    case DEFAULT_HOP_LIMIT:  
        size = sizeof(uint8_t);
        break;
    case MAX_PACKET_LIFETIME:    
        size = sizeof(uint16_t);
        break;
    case DEFAULT_PACKET_LIFETIME:   
        size = sizeof(uint16_t);
        break;
    case MAX_PACKET_DATA_RATE:   
        size = sizeof(uint16_t);
        break;
    case MAX_PACKET_DATA_RATE_EMA_BETA: 
        size = sizeof(uint16_t);
        break;
    case MAX_GN_AREA_SIZE:   
        size = sizeof(uint8_t);
        break;
    case MIN_PACKET_REPETITION_INTERVAL:  
        size = sizeof(uint16_t);
        break;
    case GEOUNICAST_FORWARDING_ALGORITHM:    
        size = sizeof(enum pico_gn_guc_forwarding_algorithm);
        break;
    case GEO_BROADCAST_FORWARDING_ALGORITHM:   
        size = sizeof(enum pico_gn_gbc_forwarding_algorithm);
        break;
    case GEOUNICAST_CBF_MIN_TIME:  
        size = sizeof(uint16_t);
        break;
    case GEOUNICAST_CBF_MAX_TIME:        
        size = sizeof(uint16_t);
        break;
    case GEOBROADCAST_CBF_MIN_TIME:    
        size = sizeof(uint16_t);
        break;
    case GEOBROADCAST_CBF_MAX_TIME:    
        size = sizeof(uint16_t);
        break;
    case DEFAULT_MAX_COMMUNICATION_RANGE:    
        size = sizeof(uint16_t);
        break;
    case GEO_AREA_LINE_FORWARDING:      
        size = sizeof(uint8_t);
        break;
    case UC_FORWARDING_PACKET_BUFFER_SIZE: 
        size = sizeof(uint8_t);
        break;
    case BC_FORWARDING_PACKET_BUFFER_SIZE: 
        size = sizeof(uint16_t);
        break;
    case CBF_PACKET_BUFFER_SIZE:   
        size = sizeof(uint16_t);
        break;
    case TRAFFIC_CLASS:    
        size = sizeof(struct pico_gn_traffic_class);
        break;
    }
    
    memcpy(pico_gn_settings_get_implementation(name), value, size);
}

void* pico_gn_settings_get_implementation(enum pico_gn_setting_name name)
{
    switch (name)
    {
    case LOCAL_GN_ADDRESS:
    {
        static struct pico_gn_address local_gn_address = (struct pico_gn_address){0};
        return (void*)&local_gn_address;
    }
    case LOCAL_ADDR_CONF_METHOD:
    {
        static enum pico_gn_address_conf_method local_address_conf_method = MANAGED;
        return (void*)&local_address_conf_method;
    }
    case PROTOCOL_VERSION:
    {
        static uint8_t protocol_version = 0;
        return (void*)&protocol_version;
    }
    case STATION_TYPE:
    {
        static enum pico_gn_station_type station_type = UNKNOWN;
        return (void*)&station_type;
    }
    case IS_MOBILE:
    {
        static uint8_t is_mobile = 0;
        return (void*)&is_mobile;
    }
    case INTERFACE_TYPE:
    {
        static enum pico_gn_interface_type interface_type = UNSPECIFIED;
        return (void*)&interface_type;
    }
    case MIN_UPDATE_FREQUENCY_LPV:
    {
        static uint16_t min_update_frequency_lpv = 0;
        return (void*)&min_update_frequency_lpv;
    }
    case PAI_INTERVAL:
    {
        static uint8_t pai_interval = 80;
        return (void*)pai_interval;
    }
    case MAX_SDU_SIZE:
    {
        static uint16_t max_sdu_size = 1398;
        return (void*)&max_sdu_size;
    }
    case MAX_GN_HEADER_SIZE:
    {
        static uint16_t max_gn_header_size = 88;
        return (void*)&max_gn_header_size;
    }
    case LIFETIME_LOCTE:
    {
        static uint16_t lifetime_locte = 20;
        return (void*)&lifetime_locte;
    }
    case SECURITY:
    {
        static enum pico_gn_security security = DISABLED;
        return (void*)&security;
    }
    case SN_DECAP_RESULT_HANDLING:
    {
        static enum pico_gn_sn_decap_result_handling sn_decap_result_handling = STRICT;
        return (void*)&sn_decap_result_handling;
    }
    case LOCATION_SERVICE_MAX_RETRANSMIT:    
    {
        static uint8_t location_service_max_retransmit = 10;
        return (void*)&location_service_max_retransmit;
    }
    case LOCATION_SERVICE_RETRANSMIT_TIMER:
    {
        static uint16_t location_service_retransmit_timer = 1000;
        return (void*)&location_service_retransmit_timer;
    }
    case LOCATION_SERVICE_PACKET_BUFFER_SIZE:
    {
        static uint16_t location_service_packet_buffer_size = 1024;
        return (void*)&location_service_packet_buffer_size;
    }
    case BEACON_SERVICE_RETRANSMIT_TIMER:
    {
        static uint16_t beacon_service_retransmit_timer = 3000;
        return (void*)&beacon_service_retransmit_timer;
    }
    case BEACON_SERVICE_MAX_JITTER:
    {
        static uint16_t beacon_service_max_jitter = 3000 / 4;
        return (void*)&beacon_service_max_jitter;
    }
    case DEFAULT_HOP_LIMIT:  
    {
        static uint8_t default_hop_limit = 10;
        return (void*)&default_hop_limit;
    }
    case MAX_PACKET_LIFETIME:            
    {
        static uint16_t max_packet_lifetime = 600;
        return (void*)&max_packet_lifetime;
    }
    case DEFAULT_PACKET_LIFETIME:    
    {
        static uint16_t default_packet_lifetime = 60;
        return (void*)&default_packet_lifetime;
    }         
    case MAX_PACKET_DATA_RATE:               
    {
        static uint16_t max_packet_data_rate = 100;
        return (void*)&max_packet_data_rate;
    }
    case MAX_PACKET_DATA_RATE_EMA_BETA:      
    {
        static uint16_t max_packet_data_rate_beta = 90;
        return (void*)&max_packet_data_rate_beta;
    }
    case MAX_GN_AREA_SIZE:   
    {
        static uint8_t max_gn_area_size = 10;
        return (void*)&max_gn_area_size;
    }
    case MIN_PACKET_REPETITION_INTERVAL:     
    {
        static uint16_t min_packet_repetition_interval = 100;
        return (void*)&min_packet_repetition_interval;
    }
    case GEOUNICAST_FORWARDING_ALGORITHM:    
    {
        static enum pico_gn_guc_forwarding_algorithm guc_forwarding_algorithm = GREEDY;
        return (void*)&guc_forwarding_algorithm;
    }
    case GEO_BROADCAST_FORWARDING_ALGORITHM: 
    {
        static enum pico_gn_gbc_forwarding_algorithm gbc_forwarding_algorithm = ADVANCED;
        return (void*)&gbc_forwarding_algorithm;
    }
    case GEOUNICAST_CBF_MIN_TIME:  
    {
        static uint16_t guc_cbf_min_time = 1;
        return (void*)&guc_cbf_min_time;
    }
    case GEOUNICAST_CBF_MAX_TIME:          
    {
        static uint16_t guc_cbf_max_time = 100;
        return (void*)&guc_cbf_max_time;
    }
    case GEOBROADCAST_CBF_MIN_TIME:    
    {
        static uint16_t gbc_cbf_min_time = 1;
        return (void*)&gbc_cbf_min_time;
    }      
    case GEOBROADCAST_CBF_MAX_TIME:    
    {
        static uint16_t gbc_cbf_max_time = 100;
        return (void*)&gbc_cbf_max_time;
    }       
    case DEFAULT_MAX_COMMUNICATION_RANGE:    
    {
        static uint16_t default_max_communication_range = 1000;
        return (void*)&default_max_communication_range;
    }
    case GEO_AREA_LINE_FORWARDING:         
    {
        static uint8_t geo_area_line_forwarding = 1;
        return (void*)&geo_area_line_forwarding;
    }
    case UC_FORWARDING_PACKET_BUFFER_SIZE:   
    {
        static uint8_t uc_forwarding_buffer_size = 256;
        return (void*)&uc_forwarding_buffer_size;
    }
    case BC_FORWARDING_PACKET_BUFFER_SIZE:   
    {
        static uint16_t bc_forwarding_buffer_size = 1024;
        return (void*)&bc_forwarding_buffer_size;
    }
    case CBF_PACKET_BUFFER_SIZE:   
    {
        static uint16_t cbf_forwarding_buffer_size = 256;
        return (void*)&cbf_forwarding_buffer_size;
    }
    case TRAFFIC_CLASS:  
    {
        static struct pico_gn_traffic_class traffic_class = (struct pico_gn_traffic_class){0};
        return (void*)&traffic_class;
    }
    }
}

void *pico_gn_settings_get(enum pico_gn_setting_name name)
{
    return pico_gn_settings_get_implementation(name);
}
void pico_gn_settings_set(enum pico_gn_setting_name name, void *value)
{
    pico_gn_settings_set_implementation(name, value);
}
