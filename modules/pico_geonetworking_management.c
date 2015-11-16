#include "pico_geonetworking_management.h"

struct pico_gn_management_interface pico_gn_mgmt_interface = {
    .get_time = NULL,
    .get_position = NULL,
};