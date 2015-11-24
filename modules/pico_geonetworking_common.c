#include "pico_geonetworking_common.h"
#include "pico_geonetworking_management.h"
#include "pico_geonetworking_guc.h"

#include "pico_config.h"
#include "pico_frame.h"
#include "pico_stack.h"
#include "pico_eth.h"
#include "pico_addressing.h"
#include "pico_tree.h"

#include <math.h>

#define PICO_GN_HEADER_COUNT    7 // Maximum number of extended headers defined. Used for the creation of lookup tables.
#define PICO_GN_SUBHEADER_COUNT 3 // Maximum number of extended sub-headers defined. Used for the creation of lookup tables.

/* INTERFACE PROTOCOL DEFINITION */

static struct pico_queue gn_in  = {0}; // Incoming frame queue
static struct pico_queue gn_out = {0}; // Outgoing frame queue

volatile struct pico_gn_header_info *next_alloc_header_type = NULL;

const struct pico_gn_traffic_class pico_gn_traffic_class_default = {0};

struct pico_protocol pico_proto_geonetworking = {
    .name = "geonetworking",
    .proto_number = PICO_PROTO_GEONETWORKING,
    .layer = PICO_LAYER_NETWORK,
    .alloc = pico_gn_alloc,
    .process_in = pico_gn_process_in,
    .process_out = pico_gn_process_out,
    .push = pico_gn_frame_sock_push,
    .q_in = &gn_in,
    .q_out = &gn_out,
};

const struct pico_gn_header_info header_info_invalid = {
    .header     = 0,
    .subheader  = 0,
    .size       = 0,
    .offsets    = {
        .sequence_number = -1,
        .timestamp       = -1,
        .source_address  = -1,
    },
    .in         = NULL,
    .out        = NULL,
    .push       = NULL,
    .alloc      = NULL,
};

PICO_TREE_DECLARE(pico_gn_dev_link, pico_gn_link_compare);

PICO_TREE_DECLARE(pico_gn_loct, pico_gn_locte_compare);

/* LOCAL ADDRESS AND DEVICE LINK FUCNTIONS */

int pico_gn_link_add(struct pico_device *dev, enum pico_gn_address_conf_method method, uint8_t station_type, uint16_t country_code)
{
    struct pico_gn_link *link = PICO_ZALLOC(PICO_SIZE_GNLINK);
    int addr_create_result;
    
    if (!link)
    {
        pico_err = PICO_ERR_ENOMEM;
        return -1;
    }
    
    link->dev = dev;
    link->address.value = 0;
    
    switch (method)
    {
    case AUTO: 
        addr_create_result = pico_gn_create_address_auto(&link->address, station_type, country_code);
        break;
    case MANAGED:
        addr_create_result = pico_gn_create_address_managed(&link->address);
        break;
    case ANONYMOUS:
        addr_create_result = pico_gn_create_address_anonymous(&link->address);
        break;
    default:
        addr_create_result = -1;
        break;
    }
    
    if (!addr_create_result)
        pico_tree_insert(&pico_gn_dev_link, link);
    
    return addr_create_result;
}

uint64_t i = 0x123;
int pico_gn_create_address_auto(struct pico_gn_address* result, uint8_t station_type, uint16_t country_code)
{
    PICO_SET_GNADDR_MANUAL(result->value, 0); // Should be 1?
    PICO_SET_GNADDR_STATION_TYPE(result->value, station_type);
    PICO_SET_GNADDR_COUNTRY_CODE(result->value, country_code);
    //PICO_SET_GNADDR_MID(result->value, ((((uint64_t)pico_rand()) << 32) | ((uint64_t)pico_rand()))); // DYNAMIC RANDOM
    PICO_SET_GNADDR_MID(result->value, (long_long_be(i++) >> 16)); // STATIC
    
    return 0;
}

int pico_gn_create_address_managed(struct pico_gn_address* result)
{
    IGNORE_PARAMETER(result);
    return -1;
}

int pico_gn_create_address_anonymous(struct pico_gn_address* result)
{
    IGNORE_PARAMETER(result);
    return -1;
}

/* FRAME ALLOCATION FUNTIONS */

struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size)
{
    struct pico_frame *f = NULL;
    IGNORE_PARAMETER(self);
    
    // Make sure the next alloc header is valid.
    if (next_alloc_header_type && next_alloc_header_type != &header_info_invalid)
    {
        f = next_alloc_header_type->alloc(size);

        // If there was enough memory, set the headers positions and their lengths.
        if (f)
        {
            uint16_t size_gn = PICO_SIZE_GNHDR + next_alloc_header_type->size;
            
            f->datalink_hdr  = f->buffer;
            f->net_hdr       = f->buffer + PICO_SIZE_ETHHDR; // This should be changed to the size of the underlying LLC protocol. Currently only Ethernet is supported, so this is sufficient for now.
            f->net_len       = size_gn;
            f->transport_hdr = f->net_hdr + size_gn;
            f->transport_len = size;
            f->len           = size + size_gn;
        }
    }
    else
    {
        // TODO: set error code to something
    }
    
    return f;
}

/* PROCESS IN FUNCTIONS */

int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f)
{
    struct pico_gn_header *h = (struct pico_gn_header*)f->net_hdr;
    int64_t extended_length = pico_gn_find_extended_header_length(h);
    const struct pico_gn_header_info *info; 
    IGNORE_PARAMETER(self);
        
    if (extended_length < 0)
    {
        dbg("No header or an invalid extended header.\n");
        // TODO: Set error code to something.
        return -1;
    }
    
    f->transport_hdr = f->net_hdr + PICO_SIZE_GNHDR + extended_length;
    f->transport_len = (uint16_t)(f->buffer_len - h->common_header.payload_length - PICO_SIZE_GNHDR - (uint16_t)extended_length);
    f->net_len = (uint16_t)(PICO_SIZE_GNHDR + (uint16_t)extended_length);
    
    // BASIC HEADER PROCESSING
    // Check if the GeoNetworking version of the received packet is compatible.
    if (PICO_GET_GNBASICHDR_VERSION(h->basic_header.vnh) != PICO_GN_PROTOCOL_VERSION)
    {
        dbg("Invalid GeoNetworking protocol version: %d.\n", PICO_GET_GNBASICHDR_VERSION(h->basic_header.vnh));
        pico_frame_discard(f);
        return 0;
    }
    
    switch (PICO_GET_GNBASICHDR_NEXT_HEADER(h->basic_header.vnh))
    {
    case BH_ANY: // The protocol states that an 'Any' should be treated as a Common Header.
    case COMMON:
        break; // Just continue execution of the function.
    case SECURED:
    default: 
        // Packet is a Secured Packet (2) which is not supported yet, or invalid (>2). Throw it away
        dbg("The header following the Basic Header is either a secured header (not supported) or invalid.\n");
        pico_frame_discard(f);
        return 0;
    }
    
    // COMMON HEADER PROCESSING
    // Check if the Maximum Hop Limit is reached, if so throw it away.
    if (h->common_header.maximum_hop_limit < h->basic_header.remaining_hop_limit)
    {
        dbg("Maximimum hop limit of the packet is exceeded.\n");
        pico_frame_discard(f);
        return 0;
    }
    
    // TODO: Process the Broadcast forwarding packet buffer (page 41)
    
    // EXTENDED HEADER PROCESSING    
    info = pico_gn_get_header_info(h);
    
    if (info && info->in)
    {
        return info->in(f);
    }
    else
    {
        pico_frame_discard(f);
        return -1;
    }
}

/* PROCESS OUT FUNCTIONS */

int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f)
{
    IGNORE_PARAMETER(self);
    IGNORE_PARAMETER(f);
    
    dbg("pico_gn_process_out.\n");
    
    //TODO: implement function
    return -1;
}

/* SOCKET PUSH FUNCTIONS*/

int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    /*struct pico_gn_data_request *request          = (struct pico_gn_data_request *)f->info;
    struct pico_gn_address      *destination_addr = request->destination;
    struct pico_gn_lpv          *destination_pv   = NULL;
    struct pico_tree_node       *index            = NULL;
    uint64_t                     next_hop_mid     = 0;
    IGNORE_PARAMETER(self);
    
    if (!request) // || !f->sock this check should be added when a GeoNetworking comparable socket is available. )
    {
        pico_err = PICO_ERR_EINVAL; // Correct error code?
        pico_frame_discard(f);
        return -1;
    }
    
    // Check whether the entry of the position vector for DE in its LocT is valid.
    pico_tree_foreach(index, &pico_gn_loct) {
        struct pico_gn_location_table_entry *entry = (struct pico_gn_location_table_entry*)index->keyValue;
        
        if (pico_gn_address_equals(destination_addr, entry->address))
        {
            destination_pv = entry->position_vector;
            break;
        }
    }
    
    // Check if an entry was found in the 
    if (!destination_pv)
    {
        // TODO: Invoke the location service.
        
        // Discard the packet, as the protocol states.
        pico_frame_discard(f);
        return -1;
    }
    
    //Determine the MID of the next hop
    switch (PICO_GN_GUC_FORWARDING_ALGORITHM)
    {
    case PICO_GN_GUC_GREEDY_FORWARDING_ALGORITHM:
            
    case PICO_GN_GUC_CONTENTION_BASED_FORWARDING_ALGORITHM:
        default:
    }
    
    // Basic Header and Common Header processing will probably be implemented here.
    
    return pico_enqueue(&gn_out, f);*/
}


/* LOCATION TABLE FUNCTIONS */
struct pico_gn_location_table_entry* pico_gn_loct_find(struct pico_gn_address *address)
{
    if (address)
    {
        struct pico_tree_node *index;

        pico_tree_foreach(index, &pico_gn_loct) {
            struct pico_gn_location_table_entry *entry = (struct pico_gn_location_table_entry*)index->keyValue;

            if (pico_gn_address_equals(address, entry->address))
                return entry;
        }
    }
    
    return NULL;
}

int pico_gn_loct_update(struct pico_gn_address *address, struct pico_gn_lpv *vector, uint8_t is_neighbour, uint16_t sequence_number, uint8_t station_type, uint32_t timestamp)
{
    struct pico_gn_location_table_entry *entry = pico_gn_loct_find(address);
    
    if (!entry)
    {
        // TODO: Set error code (to?)
        return -1;
    }
    
    entry->position_vector = *vector;
    entry->is_neighbour = is_neighbour;
    entry->ll_address = PICO_GET_GNADDR_MID(address->value);
    entry->location_service_pending = 0;
    entry->sequence_number = sequence_number;
    entry->station_type = station_type;
    entry->timestamp = timestamp;
    
    return 0;
}

struct pico_gn_location_table_entry *pico_gn_loct_add(struct pico_gn_address *address)
{
    if (address)
    {
        struct pico_gn_location_table_entry *entry = pico_gn_loct_find(address);

        if (!entry)
        {
            entry = PICO_ZALLOC(PICO_SIZE_GNLOCTE);

            if (!entry)
            {
                pico_err = PICO_ERR_ENOMEM;
                return NULL;
            }

            memset(entry, 0, sizeof(PICO_SIZE_GNLOCTE));
            entry->address = PICO_ZALLOC(PICO_SIZE_GNADDRESS);

            if (!entry->address)
            {
                pico_err = PICO_ERR_ENOMEM;
                PICO_FREE(entry);
                return NULL;
            }

            *entry->address = *address;

            if (pico_tree_insert(&pico_gn_loct, entry) == &LEAF)
                return NULL;
        }

        return entry;
    }
    
    return NULL;
}

/* HELPER FUNCTIONS */

int64_t pico_gn_find_extended_header_length(struct pico_gn_header *header)
{
    const struct pico_gn_header_info *info = pico_gn_get_header_info(header);
    
    if (info != &header_info_invalid)
        return (int64_t)info->size;
    else
        return -1;
}

int pico_gn_detect_duplicate_sntst_packet(struct pico_frame *f)
{
    struct pico_gn_address *source = pico_gn_fetch_frame_source_address(f);
    int32_t sn_current, sn_previous;
    int64_t tst_current, tst_previous;
    
    if (!source)
        return -1;
    
    if ((sn_current = pico_gn_fetch_frame_sequence_number(f)) < 0)
        return -1;
    
    if ((tst_current = pico_gn_fetch_frame_timestamp(f)) < 0)
        return -1;
    
    // Try to find the last sequence number and timestamp in the LocT.
    if (((sn_previous = pico_gn_fetch_loct_sequence_number(source)) < 0) ||
        ((tst_previous = pico_gn_fetch_loct_timestamp(source)) < 0))
    {
        // TODO: Add this item to the LocT?? Or update it?
        
        // Not found in the LocT, this means it is impossible that the packet is a duplicate.
        return 0;
    }
    else
    {
        // Found in the LocT, do the comparison as the protocol describes.
        if (((tst_current > tst_previous) && ((tst_current - tst_previous) <= (UINT32_MAX / 2))) ||
            ((tst_previous > tst_current) && ((tst_previous - tst_current) > (UINT32_MAX / 2))))
        {
            // Timestamp of the new packet is greater than that of the previous packet. This means the new packet is not a duplicate.
            // Update the timestamp and sequence number of this GeoNetworking address in it's LocTE.
            struct pico_gn_location_table_entry *entry = pico_gn_loct_find(source);
            entry->timestamp = (uint32_t)tst_current; // Is this conversion safe?
            entry->sequence_number = (uint16_t)sn_current; // Is this conversion safe?
            
            return 0;
        }
        else if (tst_current == tst_previous)
        {
            // Timestamp of the new packet is equal to that of the previous packet.
            if (((sn_current > sn_previous) && ((sn_current - tst_previous) <= (UINT16_MAX / 2))) ||
                ((sn_previous > sn_current) && ((sn_previous - tst_current) > (UINT16_MAX / 2))))
            {
                // Sequence number of the new packet is greater than that of the previous packet. This means the new packet is not a duplicate.
                // Update the timestamp and sequence number of this GeoNetworking address in it's LocTE.
                struct pico_gn_location_table_entry *entry = pico_gn_loct_find(source);
                entry->timestamp = (uint32_t)tst_current; // Is this conversion safe?
                entry->sequence_number = (uint16_t)sn_current; // Is this conversion safe?
                
                return 0;
            }
            else
            {
                // Sequence number of the new packet is not greater than that of the previous packet. This means the packet is a duplicate.
                return 1;
            }
        }
        else
        {
            // Timestamp of the new packet is not greater than that of the previous packet. This means the packet is a duplicate.
            return 1;
        }
    }
}

int pico_gn_detect_duplicate_tst_packet(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

struct pico_gn_address *pico_gn_fetch_frame_source_address(struct pico_frame *f)
{
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    const struct pico_gn_header_info *info = pico_gn_get_header_info(header);
    
    if (info && info->offsets.source_address >= 0)
        return ((struct pico_gn_address *)(f->net_hdr + PICO_SIZE_GNHDR + info->offsets.source_address));
    else
        return NULL;
}

int32_t pico_gn_fetch_frame_sequence_number(struct pico_frame *f)
{
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    const struct pico_gn_header_info *info = pico_gn_get_header_info(header);
    
    if (info && info->offsets.sequence_number >= 0)
        return ((uint16_t *)(f->net_hdr + PICO_SIZE_GNHDR + info->offsets.sequence_number))[0];
    else
        return -1;
}

int64_t pico_gn_fetch_frame_timestamp(struct pico_frame *f)
{
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    const struct pico_gn_header_info *info = pico_gn_get_header_info(header);
    
    if (info && info->offsets.sequence_number >= 0)
        return ((uint32_t *)(f->net_hdr + PICO_SIZE_GNHDR + info->offsets.timestamp))[0];
    else
        return -1;
}

int32_t pico_gn_fetch_loct_sequence_number(struct pico_gn_address *addr)
{
    struct pico_gn_location_table_entry *entry = pico_gn_loct_find(addr);
    
    if (!entry)
        return -1;
    else 
        return entry->sequence_number;
}

int64_t pico_gn_fetch_loct_timestamp(struct pico_gn_address *addr)
{
    struct pico_gn_location_table_entry *entry = pico_gn_loct_find(addr);
    
    if (!entry)
        return -1;
    else 
        return entry->timestamp;
}

int pico_gn_link_compare(void *a, void *b)
{
    struct pico_gn_link *link_a = (struct pico_gn_link*)a;
    struct pico_gn_link *link_b = (struct pico_gn_link*)b;
    
    // If the following code is correct.
    // it is the same as in the pico_gn_locte_compare function. Replace these two with a single function.
    if (pico_gn_address_equals(&link_a->address, &link_b->address))
        return 0;
    
    if (link_a->address.value < link_b->address.value)
        return -1;
    else if (link_a->address.value > link_b->address.value)
        return 1;
    else 
        return 0;
}

int pico_gn_locte_compare(void *a, void *b)
{
    struct pico_gn_location_table_entry *locte_a = (struct pico_gn_location_table_entry*)a;
    struct pico_gn_location_table_entry *locte_b = (struct pico_gn_location_table_entry*)b;
    
    if (locte_a->address->value < locte_b->address->value)
        return -1;
    else if (locte_a->address->value > locte_b->address->value)
        return 1;
    else 
        return 0;
}

int pico_gn_address_equals(struct pico_gn_address *a, struct pico_gn_address *b)
{
    return PICO_GET_GNADDR_MID(a->value) == PICO_GET_GNADDR_MID(b->value);
}

const struct pico_gn_header_info *pico_gn_get_header_info(struct pico_gn_header *header)
{
    // Lookup table with all the specific header information (processing functions and specific field offsets).
    // The invalid fields are not implemented yet or are actually invalid.
    static const struct pico_gn_header_info *lookup[PICO_GN_HEADER_COUNT][PICO_GN_SUBHEADER_COUNT] = 
    {
        /* ANY,      INVALID,  INVALID  */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},
        /* BEACON,   INVALID,  INVALID  */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},
        /* GUC,      INVALID,  INVALID  */ {&guc_header_type,     &header_info_invalid, &header_info_invalid},
        /* GAC-circ, GAC-rect, GAC-elip */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},
        /* GBC-circ, GBC-rect, GBC-elip */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},
        /* TSB-SHB,  TSB-MHP,  INVALID  */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},
        /* LS-REQ,   LS-RESP,  INVALID  */ {&header_info_invalid, &header_info_invalid, &header_info_invalid},   
    };
    
    uint8_t header_index    = PICO_GET_GNCOMMONHDR_HEADER(header->common_header.header);
    uint8_t subheader_index = PICO_GET_GNCOMMONHDR_SUBHEADER(header->common_header.header);

    if (header_index >= PICO_GN_HEADER_COUNT || subheader_index >= PICO_GN_SUBHEADER_COUNT)
        return &header_info_invalid;    
    else
        return lookup[header_index][subheader_index];
}

uint16_t pico_gn_get_sequence_number(void)
{
    static uint16_t sequence_number = 0;
    
    if (sequence_number == UINT16_MAX)
        sequence_number = 0;
    
    return ++sequence_number;
}

int pico_gn_get_current_time(uint32_t *result)
{
    // epoch since 2004-1-1 in milliseconds.
    static const uint64_t epoch = 1072915200ul * 1000ul;
    
    if (pico_gn_mgmt_interface.get_time)
    {
        *result = (uint32_t)((pico_gn_mgmt_interface.get_time() - epoch) % UINT32_MAX);
        return 0;
    }
    else
    {
        return -1;
    }
}

int pico_gn_get_position(struct pico_gn_local_position_vector *result)
{
    if (pico_gn_mgmt_interface.get_position)
    {
        struct pico_gn_local_position_vector position = pico_gn_mgmt_interface.get_position();
        memcpy(result, &position, PICO_SIZE_GNLOCAL_POSITION_VECTOR);
        return 0;
    }
    else
    {
        return -1;
    }
}

#define TO_RAD (3.1415926536 / 180)
double pico_gn_calculate_distance(int32_t lat_a, int32_t long_a, int32_t lat_b, int32_t long_b)
{
    static const double r = 6371;
    double th1 = (double)lat_a / 10000;
    double ph1 = (double)long_a / 10000;
    
    double th2 = (double)lat_b / 10000;
    double ph2 = (double)long_b / 10000;
   
    double dx, dy, dz;
    ph1 -= ph2;
    ph1 *= TO_RAD, th1 *= TO_RAD, th2 *= TO_RAD;

    dz = sin(th1) - sin(th2);
    dx = cos(ph1) * cos(th1) - cos(th2);
    dy = sin(ph1) * cos(th1);
    
    return asin(sqrt(dx * dx + dy * dy + dz * dz) / 2) * 2 * r;
}
