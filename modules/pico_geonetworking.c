#include "pico_geonetworking.h"
#include "pico_frame.h"
#include "pico_eth.h"
#include "pico_tree.h"

#define PICO_GN_HEADER_TYPE_ANY                            0
#define PICO_GN_HEADER_TYPE_BEACON                         1
#define PICO_GN_HEADER_TYPE_GEOUNICAST                     2
#define PICO_GN_HEADER_TYPE_GEOANYCAST                     3
#define PICO_GN_HEADER_TYPE_GEOBROADCAST                   4
#define PICO_GN_HEADER_TYPE_TOPOLOGICALLY_SCOPED_BROADCAST 5
#define PICO_GN_HEADER_TYPE_LOCATION_SERVICE               6

#define PICO_GN_SUBHEADER_TYPE_SINGLE_HOP 0
#define PICO_GN_SUBHEADER_TYPE_MULTI_HOP  1

#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_CIRCLE    0
#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_RECTANGLE 1
#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_ELIPSE    2

#define PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_REQUEST 0
#define PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_RESONSE 1

/* INTERFACE PROTOCOL DEFINITION */

static struct pico_queue in  = {0};
static struct pico_queue out = {0};

struct pico_protocol pico_proto_geonetworking = {
    .name = "geonetworking",
    .proto_number = PICO_PROTO_GEONETWORKING,
    .layer = PICO_LAYER_NETWORK,
    .alloc = pico_gn_alloc,
    .process_in = pico_gn_process_in,
    .process_out = pico_gn_process_out,
    .push = pico_gn_frame_sock_push,
    .q_in = &in,
    .q_out = &out,
};

/* LOCATION TABLE DEFINITION */

struct pico_tree pico_gn_location_table = { &LEAF, &pico_gn_locte_compare };

/* FRAME ALLOCATION FUNTIONS */

struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size)
{
    struct pico_frame *f = pico_frame_alloc(PICO_SIZE_ETHHDR + PICO_SIZE_GNHDR /* + Specific extended header size */ + size);
    IGNORE_PARAMETER(self);
    
    if (!f)
        return NULL;
    
    f->datalink_hdr = f->buffer;
    f->net_hdr = f->buffer + PICO_SIZE_ETHHDR;
    f->net_len = PICO_SIZE_GNHDR /* + Specific extended header size */;
    f->transport_hdr = f->net_hdr + PICO_SIZE_GNHDR /* + Specific extended header size */;
    f->transport_len = size;
    f->len = size + PICO_SIZE_GNHDR /* + Specific extended header size */;
    return f;
}

/* PROCESS IN FUNCTIONS */

int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f)
{
    struct pico_gn_header *h = (struct pico_gn_header*) f->net_hdr;
    int extended_length = pico_gn_find_extended_header_length(h);
    
    IGNORE_PARAMETER(self);
        
    if (!h || extended_length < 0)
        return -1;
    
    f->transport_hdr = f->net_hdr + PICO_SIZE_GNHDR + extended_length;
    // Not sure if the next line is correct.
    f->transport_len = (uint16_t)(f->buffer_len - h->common_header.payload_length - PICO_SIZE_GNHDR - (uint16_t)extended_length);
    f->net_len = (uint16_t)(PICO_SIZE_GNHDR + (uint16_t)extended_length);
    
    // BASIC HEADER PROCESSING
    // Check if the GeoNetworking version of the received packet is compatible.
    if (h->basic_header.version != PICO_GN_PROTOCOL_VERSION)
    {
        pico_frame_discard(f);
        return 0;
    }
    
    // Check if the next header is a Common Header
    if (!(h->basic_header.next_header == 0 ||
          h->basic_header.next_header == 1))
    { 
        // Packet is a Secured Packet (2), or invalid (>2). Throw it away
        pico_frame_discard(f);
        return 0;
    }
    
    // COMMON HEADER PROCESSING
    // Check if the Maximum Hop Limit is reached, if so throw it away.
    if (h->common_header.maximum_hop_limit < h->basic_header.remaining_hop_limit)
    {
        pico_frame_discard(f);
        return 0;
    }
    
    // TODO: Process the Broadcast forwarding packet buffer (page 41)
    
    // EXTENDED HEADER PROCESSING
    switch (h->common_header.header)
    {
        case PICO_GN_HEADER_TYPE_BEACON: return pico_gn_process_beacon_in(f);
        case PICO_GN_HEADER_TYPE_GEOUNICAST: return pico_gn_process_guc_in(f);
        case PICO_GN_HEADER_TYPE_GEOANYCAST: return pico_gn_process_gac_in(f);
        case PICO_GN_HEADER_TYPE_GEOBROADCAST: return pico_gn_process_gbc_in(f);
        case PICO_GN_HEADER_TYPE_TOPOLOGICALLY_SCOPED_BROADCAST: 
            switch (h->common_header.subheader)
            {
                case PICO_GN_SUBHEADER_TYPE_MULTI_HOP: return pico_gn_process_mh_in(f); 
                case PICO_GN_SUBHEADER_TYPE_SINGLE_HOP: return pico_gn_process_sh_in(f); 
                default: // Invalid sub-header, discard the packet
                    pico_frame_discard(f);
                    return 0;
            }
        case PICO_GN_HEADER_TYPE_LOCATION_SERVICE: return pico_gn_process_ls_in(f);
        case PICO_GN_HEADER_TYPE_ANY:
        default: // Invalid header type, discard the packet
            pico_frame_discard(f);
            return 0;
    }
}

int pico_gn_process_beacon_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_process_guc_in(struct pico_frame *f)
{
    
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_gac_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_process_gbc_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_process_mh_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_process_sh_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_process_ls_in(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

/* PROCESS OUT FUNCTIONS */

int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

/* SOCKET PUSH FUNCTIONS*/

int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

/* HELPER FUNCTIONS */

int pico_gn_find_extended_header_length(struct pico_gn_header *header)
{
    switch (header->common_header.header)
    {
        case PICO_GN_HEADER_TYPE_ANY: return 0;
        case PICO_GN_HEADER_TYPE_BEACON: return PICO_SIZE_BEACONHDR;
        case PICO_GN_HEADER_TYPE_GEOUNICAST: return PICO_SIZE_GUCHDR;
        case PICO_GN_HEADER_TYPE_GEOANYCAST: return PICO_SIZE_GBCHDR;
        case PICO_GN_HEADER_TYPE_GEOBROADCAST: return PICO_SIZE_GBCHDR;
        case PICO_GN_HEADER_TYPE_TOPOLOGICALLY_SCOPED_BROADCAST:
            switch (header->common_header.subheader)
            {
                case PICO_GN_SUBHEADER_TYPE_MULTI_HOP: return PICO_SIZE_TSCHDR;
                case PICO_GN_SUBHEADER_TYPE_SINGLE_HOP: return PICO_SIZE_SHBHDR;
                default: return -1;
            }
        case PICO_GN_HEADER_TYPE_LOCATION_SERVICE:
            switch (header->common_header.subheader)
            {
                case PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_REQUEST: return PICO_SIZE_BEACONHDR;
                case PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_RESONSE: return PICO_SIZE_BEACONHDR;
                default: return -1;
            }
        default: return -1;
    }
}

int pico_gn_detect_duplicate_SNTST_packet(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_detect_duplicate_TST_packet(struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

int pico_gn_locte_compare(void *a, void *b)
{
    struct pico_gn_lcation_table_entry *locte_a = (struct pico_gn_lcation_table_entry*)a;
    struct pico_gn_lcation_table_entry *locte_b = (struct pico_gn_lcation_table_entry*)b;
    
    if (locte_a->address->ll_address < locte_b->address->ll_address)
        return -1;
    else if (locte_a->address->ll_address > locte_b->address->ll_address)
        return 1;
    else 
        return 0;
}