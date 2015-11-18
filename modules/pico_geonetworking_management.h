/* 
 * File:   pico_gn_management.h
 * Author: stefan
 *
 * Created on November 12, 2015, 11:52 AM
 */

#ifndef INCLUDE_PICO_GEONETWORKING_MANAGEMENT
#define INCLUDE_PICO_GEONETWORKING_MANAGEMENT

#include "pico_config.h"


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

#endif /* INCLUDE_PICO_GEONETWORKING_MANAGEMENT */

