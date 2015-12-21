#include "pico_geonetworking_common.h"
#include "pico_geonetworking_management.h"
#include "pico_geonetworking_guc.h"

#include "pico_config.h"
#include "pico_frame.h"
#include "pico_stack.h"
#include "pico_eth.h"
#include "pico_addressing.h"
#include "pico_tree.h"

#define PICO_GN_HEADER_COUNT    7 // Maximum number of extended headers defined. Used for the creation of lookup tables.
#define PICO_GN_SUBHEADER_COUNT 3 // Maximum number of extended sub-headers defined. Used for the creation of lookup tables.

/* INTERFACE PROTOCOL DEFINITION */

static struct pico_queue gn_in  = {0}; // Incoming frame queue
static struct pico_queue gn_out = {0}; // Outgoing frame queue

struct pico_gn_header_info *next_alloc_header_type = NULL;

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

int pico_gn_create_address_auto(struct pico_gn_address* result, uint8_t station_type, uint16_t country_code)
{
    static uint64_t i = 0x123;
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
        
    f->start = (uint8_t*) f->net_hdr;
    
    // If there is something special to do for specific headers, do it in the process_out specific function.
    // Currently, for guc, there is nothing to do.
    
    return pico_sendto_dev(f);
}

/* SOCKET PUSH FUNCTIONS*/

int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    struct pico_gn_data_request *request = (struct pico_gn_data_request *)f->info;
    struct pico_gn_header       *header  = (struct pico_gn_header*)f->net_hdr;
    IGNORE_PARAMETER(self);
    
    if (!request || next_alloc_header_type == &header_info_invalid) // || !f->sock this check should be added when a GeoNetworking comparable socket is available. )
    {
        pico_err = PICO_ERR_EINVAL; // Correct error code?
        pico_frame_discard(f);
        return -1;
    }
    
    // Set the fields of the Basic Header
    header->basic_header.vnh = 0;
    /*⬑*/ PICO_SET_GNBASICHDR_VERSION(header->basic_header.vnh, PICO_GN_PROTOCOL_VERSION);
    /*⬑*/ PICO_SET_GNBASICHDR_NEXT_HEADER(header->basic_header.vnh, COMMON);
    header->basic_header.reserved = 0;
    header->basic_header.lifetime = request->lifetime;
    header->basic_header.remaining_hop_limit = request->maximum_hop_limit;
    
    // Set the fields of the Common Header
    header->common_header.next_header = 0;
    PICO_SET_GNCOMMONHDR_NEXT_HEADER(header->common_header.next_header, request->upper_proto); // TODO: this should be set to CM_ANY if request->type is BEACON.
    PICO_SET_GNCOMMONHDR_HEADER(header->common_header.header, request->type.header);
    PICO_SET_GNCOMMONHDR_SUBHEADER(header->common_header.header, request->type.subheader);
    header->common_header.traffic_class = request->traffic_class; 
    header->common_header.flags = (*((uint8_t*)pico_gn_settings_get(IS_MOBILE))) ? (1 << 7) : 0;
    header->common_header.payload_length = short_be(f->payload_len); // TODO: set to 0 for BEACON and Location Service packets
    header->common_header.maximum_hop_limit = request->maximum_hop_limit; // TODO: set to 1 for SHB, set to default constant for Location Service packets.
    header->common_header.reserved_2 = 0;
    
    return next_alloc_header_type->push(f);
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
    
    uint64_t mid_a = PICO_GET_GNADDR_MID(link_a->address.value);
    uint64_t mid_b = PICO_GET_GNADDR_MID(link_b->address.value);
            
    if (mid_a < mid_b)
        return 1;
    else if (mid_a > mid_b)
        return -1;
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
    if (header)
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
    
    return &header_info_invalid; 
}

uint16_t pico_gn_get_next_sequence_number(void)
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

#define MINDIFF 2.25e-308
double pico_gn_sqrt(double value)
{
    double root = value / 3, last, diff = 1;
    
    if (value <= 0) 
        return 0;
    
    do 
    {
        last = root;
        root = (root + value / root) / 2;
        diff = root - last;
    } while (diff > MINDIFF || diff < -MINDIFF);
    
    return root;
}

double pico_gn_abs(double value)
{
    if (value < 0)
        return -value;
    else 
        return value;
}

int pico_gn_factorial(int value)
{
    if(value <= 1)
        return 1;
    else
        return (value * pico_gn_factorial(value - 1));
}

double pico_gn_pow(double x, int y)
{
    double temp;
    
    if( y == 0)
       return 1;
    
    temp = pico_gn_pow(x, y / 2);     
    
    if (y % 2 == 0)
        return temp*temp;
    else
    {
        if(y > 0)
            return x * temp * temp;
        else
            return (temp * temp) / x;
    }
}


double pico_gn_cos(double value)
{
    unsigned int i;
    double sum = 0.0;

    for (i = 0; i < 10; i++)
        sum += pico_gn_pow(-1, i) * pico_gn_pow(value, 2*i) / pico_gn_factorial(2*i);

    return sum;
}

#define TO_RADIANS(x) ((3.14159265358979323846 / 180) * (x))
int32_t pico_gn_calculate_distance(int32_t lat_a, int32_t long_a, int32_t lat_b, int32_t long_b)
{
    static const int32_t converter = 100000;
    static const double earth_radius = 6372.8;
    
    double lat_a_v = ((double)lat_a) / converter;
    double lat_b_v = ((double)lat_b) / converter;
    double long_a_v = ((double)long_a) / converter;
    double long_b_v = ((double)long_b) / converter;
    
    double alpha = long_b_v - long_a_v;

    double x = TO_RADIANS(alpha) * pico_gn_cos(TO_RADIANS(lat_a_v + lat_b_v) / 2);
    double y = TO_RADIANS(lat_a_v - lat_b_v);
    
    double d = pico_gn_sqrt(x * x + y * y) * earth_radius;
    
    return ((int32_t)(d * 1000));
}

int pico_gn_is_broadcast(const uint8_t address[6])
{
    uint8_t i;
    for (i = 0; i < 6; ++i) 
        if (address[i] != 0xFF)
            return 0;
    
    return 1;
}