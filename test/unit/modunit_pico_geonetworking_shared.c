#include "pico_geonetworking_common.h"
#include "pico_geonetworking_guc.h"
#include "pico_config.h"

struct pico_frame *pico_gn_create_guc_packet(struct pico_gn_lpv source, struct pico_gn_spv destination)
{
    struct pico_frame                *frame       = NULL;
    struct pico_gn_header            *header      = NULL; 
    struct pico_gn_guc_header        *extended    = NULL;
    const struct pico_gn_header_info *header_info = &guc_header_type;
        
    next_alloc_header_type = header_info;
    
    frame    = pico_gn_alloc(&pico_proto_geonetworking, 0);
    header   = (struct pico_gn_header*)frame->net_hdr;
    extended = (struct pico_gn_guc_header*)(frame->net_hdr + PICO_SIZE_GNHDR);
    
    // Set the Basic Header
    header->basic_header.lifetime = 255;
    header->basic_header.remaining_hop_limit = 10;
    header->basic_header.reserved = 0;
    PICO_SET_GNBASICHDR_VERSION(header->basic_header.vnh, 0);
    PICO_SET_GNBASICHDR_NEXT_HEADER(header->basic_header.vnh, COMMON);
    
    // Set the Common Header
    header->common_header.flags = 0;
    header->common_header.payload_length = 0;
    header->common_header.traffic_class = pico_gn_traffic_class_default;
    PICO_SET_GNCOMMONHDR_NEXT_HEADER(header->common_header.next_header, BTP_A);
    PICO_SET_GNCOMMONHDR_HEADER(header->common_header.header, header_info->header);
    PICO_SET_GNCOMMONHDR_SUBHEADER(header->common_header.header, header_info->subheader);
    
    // Set the GeoUnicast header
    memcpy(&extended->destination, &destination, PICO_SIZE_GNSPV);
    memcpy(&extended->source, &source, PICO_SIZE_GNLPV);
    extended->sequence_number = 123;
    
    return frame;
}

struct pico_frame *pico_gn_create_guc_packet_sn(struct pico_gn_lpv source, struct pico_gn_spv destination, uint16_t sequence_number)
{
    struct pico_frame *frame = pico_gn_create_guc_packet(source, destination);
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(frame->net_hdr + PICO_SIZE_GNHDR);
    
    extended->sequence_number = sequence_number;
    
    return frame;
}