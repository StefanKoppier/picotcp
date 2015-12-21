/* 
 * File:   pico_gn_management.h
 * Author: stefan
 *
 * Created on November 12, 2015, 11:52 AM
 */

#ifndef INCLUDE_PICO_GEONETWORKING_MANAGEMENT
#define INCLUDE_PICO_GEONETWORKING_MANAGEMENT

#include "pico_geonetworking_common.h"

#include "pico_config.h"
#include "pico_tree.h"

#define PICO_SIZE_GNLOCAL_POSITION_VECTOR ((uint32_t)sizeof(struct pico_gn_local_position_vector))
struct pico_gn_local_position_vector
{
    int32_t  latitude;
    int32_t  longitude; 
    uint16_t speed: 15;
    uint16_t heading;
    uint32_t timestamp;
    uint8_t  accuracy: 1;
};

struct pico_gn_management_interface
{
    uint64_t                             (*get_time)     (void);
    struct pico_gn_local_position_vector (*get_position) (void);
};

extern struct pico_gn_management_interface pico_gn_mgmt_interface;

#define PICO_SHIFT_GNCONFIGURATION_SUB_GROUP   (0 << 6)
#define PICO_SHIFT_GNLOCATION_SUB_GROUP        (1 << 6)
#define PICO_SHIFT_BEACON_SERVICE_SUB_GROUP    (2 << 6)
#define PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP (3 << 6)

enum pico_gn_setting_name
{
    // Configuration settings sub group
    LOCAL_GN_ADDRESS         = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 1,
    LOCAL_ADDR_CONF_METHOD   = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 2,
    PROTOCOL_VERSION         = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 3,
    STATION_TYPE             = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 4,
    IS_MOBILE                = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 5,
    INTERFACE_TYPE           = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 6,
    MIN_UPDATE_FREQUENCY_LPV = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 7,
    PAI_INTERVAL             = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 8,
    MAX_SDU_SIZE             = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 9,
    MAX_GN_HEADER_SIZE       = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 10,
    LIFETIME_LOCTE           = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 11,
    SECURITY                 = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 12,
    SN_DECAP_RESULT_HANDLING = PICO_SHIFT_GNCONFIGURATION_SUB_GROUP + 13,
    
    // Location Service sub group
    LOCATION_SERVICE_MAX_RETRANSMIT     = PICO_SHIFT_GNLOCATION_SUB_GROUP + 1,
    LOCATION_SERVICE_RETRANSMIT_TIMER   = PICO_SHIFT_GNLOCATION_SUB_GROUP + 2,
    LOCATION_SERVICE_PACKET_BUFFER_SIZE = PICO_SHIFT_GNLOCATION_SUB_GROUP + 3,
    
    // BEACON service sub group
    BEACON_SERVICE_RETRANSMIT_TIMER = PICO_SHIFT_BEACON_SERVICE_SUB_GROUP + 1,
    BEACON_SERVICE_MAX_JITTER       = PICO_SHIFT_BEACON_SERVICE_SUB_GROUP + 2,
    
    // Packet forwarding sub group
    DEFAULT_HOP_LIMIT                  = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 1,
    MAX_PACKET_LIFETIME                = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 2,
    DEFAULT_PACKET_LIFETIME            = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 3,
    MAX_PACKET_DATA_RATE               = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 4,
    MAX_PACKET_DATA_RATE_EMA_BETA      = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 5,
    MAX_GN_AREA_SIZE                   = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 6,
    MIN_PACKET_REPETITION_INTERVAL     = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 7,
    GEOUNICAST_FORWARDING_ALGORITHM    = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 8,
    GEOBROADCAST_FORWARDING_ALGORITHM = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 9,
    GEOUNICAST_CBF_MIN_TIME            = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 10,
    GEOUNICAST_CBF_MAX_TIME            = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 11,
    GEOBROADCAST_CBF_MIN_TIME          = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 12,
    GEOBROADCAST_CBF_MAX_TIME          = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 13,
    DEFAULT_MAX_COMMUNICATION_RANGE    = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 14,
    GEO_AREA_LINE_FORWARDING           = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 15,
    UC_FORWARDING_PACKET_BUFFER_SIZE   = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 16,
    BC_FORWARDING_PACKET_BUFFER_SIZE   = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 17,
    CBF_PACKET_BUFFER_SIZE             = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 18,
    TRAFFIC_CLASS                      = PICO_SHIFT_PACKET_FORWARDING_SUB_GROUP + 19,
};

void pico_gn_settings_set_implementation(enum pico_gn_setting_name name, void *value);
void* pico_gn_settings_get_implementation(enum pico_gn_setting_name name);

void pico_gn_settings_set(enum pico_gn_setting_name name, void *value);
void *pico_gn_settings_get(enum pico_gn_setting_name name);

struct pico_gn_setting
{
    enum pico_gn_setting_name index;
    void                     *value;
};

#endif /* INCLUDE_PICO_GEONETWORKING_MANAGEMENT */

