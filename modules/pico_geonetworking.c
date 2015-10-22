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

#define PICO_GN_SUBHEADER_TYPE_TSB_SINGLE_HOP 0
#define PICO_GN_SUBHEADER_TYPE_TSB_MULTI_HOP  1

#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_CIRCLE    0
#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_RECTANGLE 1
#define PICO_GN_SUBHEADER_TYPE_GEOBROADCAST_ELIPSE    2

#define PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_REQUEST 0
#define PICO_GN_SUBHEADER_TYPE_LOCATION_SERVICE_RESONSE 1

#define PICO_GN_LOCAL_ADDRESS_CONF_MET

/* INTERFACE PROTOCOL DEFINITION */

static struct pico_queue gn_in  = {0}; // Incoming frame queue
static struct pico_queue gn_out = {0}; // Outgoing frame queue

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

/* LOCATION TABLE DEFINITION */

PICO_TREE_DECLARE(pico_gn_loct, pico_gn_locte_compare);

/* LOCAL ADDRESS AND DEVICE LINK DEFINITION */

PICO_TREE_DECLARE(pico_gn_dev_link, pico_gn_link_compare);

/* LOCAL ADDRESS AND DEVICE LINK FUCNTIONS */

int pico_gn_link_add(struct pico_device *dev, enum pico_gn_address_conf_method method, uint8_t station_type, uint16_t country_code)
{
    struct pico_gn_link *link = PICO_ZALLOC(PICO_SIZE_GNLINK);
    int error;
    
    if (!link)
    {
        pico_err = PICO_ERR_ENOMEM;
        return -1;
    }
    
    link->dev = dev;
    
    switch (method)
    {
    case AUTO: 
        error = pico_gn_create_address_auto(&link->address, station_type, country_code);
        break;
    case MANAGED:
        error = pico_gn_create_address_managed(&link->address);
        break;
    case ANONYMOUS:
        error = pico_gn_create_address_anonymous(&link->address);
        break;
    }
    
    if (error == -1)
    {
        /// TODO: set error code to something.
        return -1;
    }
    
    pico_tree_insert(&pico_gn_dev_link, &link);
}

int pico_gn_create_address_auto(struct pico_gn_address* result, uint8_t station_type, uint16_t country_code)
{
    result->manual = 0;
    result->station_type = station_type;
    result->country_code = country_code;
    result->mid = (((uint64_t)pico_rand()) << 32) | ((uint64_t)pico_rand());
    
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
    struct pico_frame *f = pico_frame_alloc(PICO_SIZE_ETHHDR + PICO_SIZE_GNHDR /* + Specific extended header size */ + size);
    IGNORE_PARAMETER(self);
    
    dbg("pico_gn_alloc.\n");
    
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
    
    dbg("pico_gn_process_in\n");
        
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
        // Packet is a Secured Packet (2) which is not supported yet, or invalid (>2). Throw it away
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
        case PICO_GN_SUBHEADER_TYPE_TSB_MULTI_HOP: return pico_gn_process_mh_in(f); 
        case PICO_GN_SUBHEADER_TYPE_TSB_SINGLE_HOP: return pico_gn_process_sh_in(f); 
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
    //struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_address *source_addr = &extended->source.short_pv.address;
    struct pico_gn_address *dest_addr = &extended->destination.address;
    struct pico_gn_location_table_entry *locte = NULL;
    
    // CHeck if the destination address of the packet is the local address
    struct pico_gn_link *entry;
    struct pico_tree_node *index;
    int found_in_devices = 0;
    
    pico_tree_foreach(index, &pico_gn_dev_link) {
        entry = (struct pico_gn_link*)index->keyValue;
        
        if (pico_gn_address_equals(dest_addr, &entry->address))
        {
            found_in_devices = 1;
            break;
        }
    }
    
    // Check whether the packet should be received or forwarded.
    if (found_in_devices)
    { // Receive, this packet is for this GeoAdhoc router. 
        
        // Check if this packet is a duplicate.
        int result = pico_gn_detect_duplicate_SNTST_packet(f);
        switch (result)
        {
        case 0: break; // Not a duplicate, continue
        case 1: // A duplicate, exit quietly.
        case -1: // Failure, exit with an error code.
            pico_frame_discard(f);
            // TODO: set error code if result == -1
            return result;
        }
        
        // Check (and possibly update) the local address for duplicate addresses.
        pico_gn_detect_duplicate_address(f);
        
        // Check if the address already has an entry for this PV.
        locte = pico_gn_loct_find(source_addr);
        
        if (!locte)
        {
            // Create an entry for the received packet.
            locte = pico_gn_loct_add(source_addr);
            
            // Check if the adding of the new LocTE was successful.
            if (!locte)
            {
                // TODO: Set error code to something
                return -1;
            }
            
            locte->is_neighbour = 0;
        }
        
        // Update the LocTE members with the newly received information.
        memcpy(&locte->position_vector, &extended->source, PICO_SIZE_GNLPV);
        locte->sequence_number = extended->sequence_number;
        locte->timestamp = extended->source.short_pv.timestamp;
        // TODO: locte->packet_data_rate = A NEW UNKNOWN VALUE;
        
        
        // TODO: Flush the Location Service packet buffer
        
        // TODO: Flush the Unicast forwarding packet buffer
        
        // TODO: Pass the payload to the upper protocol.
        
        
        
        // Everything went correctly, the transport layer protocol now has the payload.
        return 0;
    }
    else
    { // Forward, this packet is not for this GeoAdhoc router. 
        
        // TODO: Implement function
        return -1;
    }
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
    dbg("pico_gn_process_out.\n");
    
    // TODO: Implement function
    return -1;
}

/* SOCKET PUSH FUNCTIONS*/

int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}


/* LOCATION TABLE FUNCTIONS */
struct pico_gn_location_table_entry* pico_gn_loct_find(struct pico_gn_address *address)
{
    struct pico_gn_location_table_entry *entry;
    struct pico_tree_node *index;
    
    pico_tree_foreach(index, &pico_gn_loct) {
        entry = (struct pico_gn_location_table_entry*)index->keyValue;
        
        if (pico_gn_address_equals(address, entry->address))
            return entry;
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
    
    memcpy(entry->position_vector, vector, PICO_SIZE_GNLPV);
    entry->is_neighbour = is_neighbour;
    entry->ll_address = address->mid;
    entry->location_service_pending = 0;
    entry->sequence_number = sequence_number;
    entry->station_type = station_type;
    entry->timestamp = timestamp;
    
    return 0;
}

struct pico_gn_location_table_entry *pico_gn_loct_add(struct pico_gn_address *address)
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
        
        entry->address = PICO_ZALLOC(PICO_SIZE_GNADDRESS);
        
        if (!entry->address)
        {
            pico_err = PICO_ERR_ENOMEM;
            PICO_FREE(entry);
            return NULL;
        }
        
        memcpy(entry->address, address, PICO_SIZE_GNADDRESS);
        
        if (pico_tree_insert(&pico_gn_loct, entry) == &LEAF)
            return NULL;
    }
    
    return entry;
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
            case PICO_GN_SUBHEADER_TYPE_TSB_MULTI_HOP: return PICO_SIZE_TSCHDR;
            case PICO_GN_SUBHEADER_TYPE_TSB_SINGLE_HOP: return PICO_SIZE_SHBHDR;
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

void pico_gn_detect_duplicate_address(struct pico_frame *f)
{
    // TODO: implement function
}

static int pico_gn_link_compare(void *a, void *b)
{
    struct pico_gn_link *link_a = (struct pico_gn_link*)a;
    struct pico_gn_link *link_b = (struct pico_gn_link*)b;
    
    // If the following code is correct.
    // it is the same as in the pico_gn_locte_compare function. Replace these two with a single function.
    if (pico_gn_address_equals(&link_a->address, &link_b->address))
        return 0;
    
    if (link_a->address.mid < link_b->address.mid)
        return -1;
    else if (link_a->address.mid > link_b->address.mid)
        return 1;
    else 
        return 0;
}

int pico_gn_locte_compare(void *a, void *b)
{
    struct pico_gn_location_table_entry *locte_a = (struct pico_gn_location_table_entry*)a;
    struct pico_gn_location_table_entry *locte_b = (struct pico_gn_location_table_entry*)b;
    
    if (locte_a->address->mid < locte_b->address->mid)
        return -1;
    else if (locte_a->address->mid > locte_b->address->mid)
        return 1;
    else 
        return 0;
}

int pico_gn_address_equals(struct pico_gn_address *a, struct pico_gn_address *b)
{
    return a->country_code == b->country_code &&
           a->mid          == b->mid          && 
           a->manual       == b->manual       &&
           a->station_type == b->station_type;
}