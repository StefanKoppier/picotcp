/*********************************************************************
   PicoTCP. Copyright (c) 2012-2015 Altran Intelligent Systems. Some rights reserved.
   See LICENSE and COPYING for usage.

 *********************************************************************/
#ifndef INCLUDE_PICO_ADDRESSING
#define INCLUDE_PICO_ADDRESSING

#include "pico_config.h"

PACKED_STRUCT_DEF pico_ip4
{
    uint32_t addr;
};

PACKED_STRUCT_DEF pico_ip6
{
    uint8_t addr[16];
};

union pico_address
{
    struct pico_ip4 ip4;
    struct pico_ip6 ip6;
};

PACKED_STRUCT_DEF pico_eth
{
    uint8_t addr[6];
    uint8_t padding[2];
};

extern const uint8_t PICO_ETHADDR_ALL[];


PACKED_STRUCT_DEF pico_trans
{
    uint16_t sport;
    uint16_t dport;

};

/* Here are some protocols. */
#define PICO_PROTO_IPV4            0
#define PICO_PROTO_ICMP4           1
#define PICO_PROTO_IGMP            2
#define PICO_PROTO_TCP             6
#define PICO_PROTO_UDP             17
#define PICO_PROTO_BTP_A           61 // Maybe an invalid number. NOT IMPLEMENTED
#define PICO_PROTO_BTP_B           62 // Maybe an invalid number. NOT IMPLEMENTED
#define PICO_PROTO_IPV6            41
#define PICO_PROTO_ICMP6           58
#define PICO_PROTO_GEONETWORKING   60 // Maybe an invalid number.
#define PICO_PROTO_GN6ASL          63 // Maybe an invalid number. GeoNetworking to Ipv6 Adaption Sub-Layer. NOT IMPLEMENTED      


#endif
