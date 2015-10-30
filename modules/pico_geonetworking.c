#include "pico_geonetworking.h"
#include "pico_frame.h"
#include "pico_stack.h"
#include "pico_eth.h"
#include "pico_addressing.h"
#include "pico_tree.h"

#define PICO_GN_HEADER_COUNT                               7 // Maximum number of extended headers defined. Used for the creation of lookup tables.
#define PICO_GN_SUBHEADER_COUNT                            3 // Maximum number of extended sub-headers defined. Used for the creation of lookup tables.

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

#define PICO_GN_BASIC_HEADER_NEXT_HEADER_ANY     0
#define PICO_GN_BASIC_HEADER_NEXT_HEADER_COMMON  1
#define PICO_GN_BASIC_HEADER_NEXT_HEADER_SECURED 2

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

/* DEBUG PRINT FUNCTIONS */
inline void pico_gn_print_header(struct pico_gn_header* h)
{
    dbg("%d %d %d %d %d\n", 
            PICO_GET_GNBASICHDR_VERSION(h->basic_header.vnh), 
            PICO_GET_GNBASICHDR_NEXT_HEADER(h->basic_header.vnh),
            h->basic_header.reserved, 
            h->basic_header.lifetime, 
            h->basic_header.remaining_hop_limit);
}

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
    default:
        error = -1;
        break;
    }
    
    if (error == -1)
    {
        /// TODO: set error code to something.
        return -1;
    }
    
    pico_tree_insert(&pico_gn_dev_link, &link);
    return 0;
}

int pico_gn_create_address_auto(struct pico_gn_address* result, uint8_t station_type, uint16_t country_code)
{
    PICO_SET_GNADDR_MANUAL(result->value, 0); // Should be 1?
    PICO_SET_GNADDR_STATION_TYPE(result->value, station_type);
    PICO_SET_GNADDR_COUNTRY_CODE(result->value, country_code);
    PICO_SET_GNADDR_MID(result->value, ((((uint64_t)pico_rand()) << 32) | ((uint64_t)pico_rand())));
    
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
    
    if (!h || extended_length < 0)
    {
        dbg("No header or an invalid extended header.\n");
        // TODO: Set error code to something.
        return -1;
    }
    
    f->transport_hdr = f->net_hdr + PICO_SIZE_GNHDR + extended_length;
    // Not sure if the next line is correct.
    f->transport_len = (uint16_t)(f->buffer_len - h->common_header.payload_length - PICO_SIZE_GNHDR - (uint16_t)extended_length);
    f->net_len = (uint16_t)(PICO_SIZE_GNHDR + (uint16_t)extended_length);
    
    pico_gn_print_header(h); // rename print header
    
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
    case PICO_GN_BASIC_HEADER_NEXT_HEADER_ANY: // The protocol states that an 'Any' should be treated as a Common Header.
    case PICO_GN_BASIC_HEADER_NEXT_HEADER_COMMON:
        break; // Just continue execution of the function.
    case PICO_GN_BASIC_HEADER_NEXT_HEADER_SECURED:
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
    switch (PICO_GET_GNCOMMONHDR_HEADER(h->common_header.header))
    {
    case PICO_GN_HEADER_TYPE_BEACON: return pico_gn_process_beacon_in(f);
    case PICO_GN_HEADER_TYPE_GEOUNICAST: return pico_gn_process_guc_in(f);
    case PICO_GN_HEADER_TYPE_GEOANYCAST: return pico_gn_process_gac_in(f);
    case PICO_GN_HEADER_TYPE_GEOBROADCAST: return pico_gn_process_gbc_in(f);
    case PICO_GN_HEADER_TYPE_TOPOLOGICALLY_SCOPED_BROADCAST: 
        switch (PICO_GET_GNCOMMONHDR_HEADER(h->common_header.header))
        {
        case PICO_GN_SUBHEADER_TYPE_TSB_MULTI_HOP: return pico_gn_process_mh_in(f); 
        case PICO_GN_SUBHEADER_TYPE_TSB_SINGLE_HOP: return pico_gn_process_sh_in(f); 
        default: // Invalid sub-header, discard the packet
            dbg("Invalid TSB sub-header: %d.\n", PICO_GET_GNCOMMONHDR_SUBHEADER(h->common_header.header));
            pico_frame_discard(f);
            return 0;
        }
    case PICO_GN_HEADER_TYPE_LOCATION_SERVICE: return pico_gn_process_ls_in(f);
    case PICO_GN_HEADER_TYPE_ANY:
    default: // Invalid header type, discard the packet
        dbg("Invalid header type: %d.\n", PICO_GET_GNCOMMONHDR_HEADER(h->common_header.header));
        pico_frame_discard(f);
        return 0;
    }
}

int pico_gn_process_beacon_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_guc_in(struct pico_frame *f)
{
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_address *dest_addr = &extended->destination.address;
    
    // Check if the destination address of the packet is the local address
    struct pico_gn_link *entry;
    struct pico_tree_node *index;
    int found_in_devices = 0;
    
    dbg("pico_gn_process_guc_in.\n");
    
    // ACT IF A MID FIELD OF ALL 1's IS FOR THIS GEOADHOC ROUTER.
    // THIS IS NOT PART OF THE GEONETWORKING STANDARD AND MUST BE REMOVED WHEN TWHE PROTOCOL IS DEPLOYED.
    // THIS IS A POTENTIAL UNDEFINED BEHAVIOUR CAUSE IF THE DEST ADDR IS ACTUALY ALL 1's BUT NOT DETERMINDED FOR THIS GEOADHOC ROUTER
    if (PICO_GET_GNADDR_MID(dest_addr->value) == 0xFFFFFFFFFFFF)
    {
        found_in_devices = 1;
    }
    else
    {
        int i = 0;
        pico_tree_foreach(index, &pico_gn_dev_link) {
            entry = (struct pico_gn_link*)index->keyValue;

            dbg("Device: [%d] with address %ld \n", ++i, entry->address.value);

            if (pico_gn_address_equals(dest_addr, &entry->address))
            {
                found_in_devices = 1;
                break;
            }
        }
    }
    
    // Check whether the packet should be received or forwarded.
    if (found_in_devices)
        return pico_gn_process_guc_receive(f);
    else
        return pico_gn_process_guc_forward(f);
}

int pico_gn_process_guc_receive(struct pico_frame *f)
{ 
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    struct pico_gn_address *source_addr = &extended->source.short_pv.address;
    struct pico_gn_location_table_entry *locte = NULL;
    int is_duplicate = pico_gn_detect_duplicate_sntst_packet(f);

    dbg("GeoNetworking address matches this GeoAdhoc router, receive this packet.\n");
        
    // Check if this packet is a duplicate.
    switch (is_duplicate)
    {
    case 0: break; // Not a duplicate, continue
    case 1: // A duplicate, exit quietly.
    case -1: // Failure, exit with an error code.
        pico_frame_discard(f);
        // TODO: set error code if result == -1
        return is_duplicate;
    }

    // Check (and possibly update) the local address for duplicate addresses.
    pico_gn_detect_duplicate_address(f); // Maybe split to detecting and changing the address

    // Check if the address already has an entry for this PV.
    locte = pico_gn_loct_find(source_addr);
    
    if (!locte)
    {
        dbg("The GeoNetworking address was not in the LocT, trying to add it to the LocT.\n");
        // Create an entry for the received packet.
        locte = pico_gn_loct_add(source_addr);

        // Check if the adding of the new LocTE was successful.
        if (!locte)
        {
            dbg("Failed to add the item to the LocT.\n");
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
    
    dbg("Successfully added/updated to the LocT.\n");


    // TODO: Flush the Location Service packet buffer

    // TODO: Flush the Unicast forwarding packet buffer

    // Pass the payload to the one of the protocols above GeoNetworking.
    // Currently none of these protocols are implemented in picoTCP, so 
    switch (PICO_GET_GNCOMMONHDR_NEXT_HEADER(header->common_header.next_header))
    {
    case PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A:
        pico_transport_receive(f, PICO_PROTO_BTP_A);
        return 0;
    case PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B:
        pico_transport_receive(f, PICO_PROTO_BTP_B);
        return 0;
    case PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6:
        dbg("GN6ASL packet received, but GN6ASL is not implemented. The packet shall be discarded.\n");
        pico_frame_discard(f);
        return 0;
    case PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY: // GeoNetworking not specifies what to do with an 'any' transport layer protocol, throw the packet away.
    default: // Invalid 
        dbg("Invalid next header: %d, discard it.\n", PICO_GET_GNCOMMONHDR_NEXT_HEADER(header->common_header.next_header));
        pico_frame_discard(f);
        return 0;
    }
}

int pico_gn_process_guc_forward(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
        
    dbg("Received packet should be forwarded.\n");
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_gac_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_gbc_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_mh_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_sh_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

int pico_gn_process_ls_in(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}

/* PROCESS OUT FUNCTIONS */

int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f)
{
    IGNORE_PARAMETER(self);
    IGNORE_PARAMETER(f);
    
    dbg("pico_gn_process_out.\n");
    
    // TODO: Implement function
    return -1;
}

/* SOCKET PUSH FUNCTIONS*/

int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    IGNORE_PARAMETER(self);
    IGNORE_PARAMETER(f);
    
    // TODO: Implement function
    return -1;
}


/* LOCATION TABLE FUNCTIONS */
struct pico_gn_location_table_entry* pico_gn_loct_find(struct pico_gn_address *address)
{
    struct pico_tree_node *index;
    
    pico_tree_foreach(index, &pico_gn_loct) {
        struct pico_gn_location_table_entry *entry = (struct pico_gn_location_table_entry*)index->keyValue;
        
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
    entry->ll_address = PICO_GET_GNADDR_MID(address->value);
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
    // Lookup table of extended headers with the size of these headers.
    static const int lookup[PICO_GN_HEADER_COUNT][PICO_GN_SUBHEADER_COUNT] = 
    {
        /* ANY,      INVALID,  INVALID  */ {0,                   -1,                  -1},
        /* BEACON,   INVALID,  INVALID  */ {PICO_SIZE_BEACONHDR, -1,                  -1},
        /* GUC,      INVALID,  INVALID  */ {PICO_SIZE_GUCHDR,    -1,                  -1},
        /* GAC-circ, GAC-rect, GAC-elip */ {PICO_SIZE_GBCHDR,     PICO_SIZE_GBCHDR,    PICO_SIZE_GBCHDR},
        /* GBC-circ, GBC-rect, GBC-elip */ {PICO_SIZE_GBCHDR,     PICO_SIZE_GBCHDR,    PICO_SIZE_GBCHDR},
        /* TSB-SHB,  TSB-MHP,  INVALID  */ {PICO_SIZE_SHBHDR,     PICO_SIZE_TSCHDR,   -1},
        /* LS-REQ,   LS-RESP,  INVALID  */ {PICO_SIZE_LSREQHDR,   PICO_SIZE_LSRESHDR, -1},                     
    };
    
    uint8_t header_index    = PICO_GET_GNCOMMONHDR_HEADER(header->common_header.header);
    uint8_t subheader_index = PICO_GET_GNCOMMONHDR_SUBHEADER(header->common_header.header);
    
    dbg("Header index: %d. Sub-header index:%d.\n", header_index, subheader_index);
    
    if (header_index >= PICO_GN_HEADER_COUNT || subheader_index >= PICO_GN_SUBHEADER_COUNT)
        return -1;
    
    return lookup[header_index][subheader_index];
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
        if (((tst_current > tst_previous) && ((tst_current - tst_previous) > (UINT32_MAX / 2))) ||
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
            if (((sn_current > sn_previous) && ((sn_current - tst_previous) > (UINT16_MAX / 2))) ||
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
    // Lookup table of extended headers and the offset (beginning from the extended header itself) in bytes of the GeoNetworking address inside the extended header.
    // -1 means that the entry is invalid or that there is no GeoNetworking address in this type of extended header.
    static const int lookup[PICO_GN_HEADER_COUNT][PICO_GN_SUBHEADER_COUNT] = 
    {
        /* ANY,      INVALID,  INVALID  */ {-1, -1, -1},
        /* BEACON,   INVALID,  INVALID  */ {0,  -1, -1},
        /* GUC,      INVALID,  INVALID  */ {4,   0,  0},
        /* GAC-circ, GAC-rect, GAC-elip */ {4,   4,  4},
        /* GBC-circ, GBC-rect, GBC-elip */ {4,   4,  4},
        /* TSB-SHB,  TSB-MHP,  INVALID  */ {0,   4, -1},
        /* LS-REQ,   LS-RESP,  INVALID  */ {0,   4, -1},
    };
    
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    
    uint8_t header_index    = PICO_GET_GNCOMMONHDR_HEADER(header->common_header.header);
    uint8_t subheader_index = PICO_GET_GNCOMMONHDR_SUBHEADER(header->common_header.header);
    int offset = -1;
    
    if (header_index >= PICO_GN_HEADER_COUNT || subheader_index >= PICO_GN_SUBHEADER_COUNT)
        return NULL;
    
    offset = lookup[header_index][subheader_index];
    
    if (offset == -1)
        return NULL;
    
    return ((struct pico_gn_address*)(f->net_hdr + PICO_SIZE_GNHDR + offset));
}

int32_t pico_gn_fetch_frame_sequence_number(struct pico_frame *f)
{
    // Lookup table of extended headers and the offset (beginning from the extended header itself) in bytes of the sequence number inside the extended header.
    // -1 means an invalid entry, -2 means that there is no sequence number in this type of extended header.
    static const int lookup[PICO_GN_HEADER_COUNT][PICO_GN_SUBHEADER_COUNT] =
    {       
        /* ANY,      INVALID,  INVALID  */ {-2, -1, -1},
        /* BEACON,   INVALID,  INVALID  */ {-2, -1, -1},
        /* GUC,      INVALID,  INVALID  */ { 0, -1, -1},
        /* GAC-circ, GAC-rect, GAC-elip */ { 0,  0,  0},
        /* GBC-circ, GBC-rect, GBC-elip */ { 0,  0,  0},
        /* TSB-SHB,  TSB-MHP,  INVALID  */ {-2,  0, -1}, 
        /* LS-REQ,   LS-RESP,  INVALID  */ { 0,  0, -1},            
    };
    
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    
    uint8_t header_index    = PICO_GET_GNCOMMONHDR_HEADER(header->common_header.header);
    uint8_t subheader_index = PICO_GET_GNCOMMONHDR_SUBHEADER(header->common_header.header);
    int offset = -1;
    
    if (header_index >= PICO_GN_HEADER_COUNT || subheader_index >= PICO_GN_SUBHEADER_COUNT)
        return -1;
    
    offset = lookup[header_index][subheader_index];
    
    if (offset < 0)
        return -1;
    
    return ((uint16_t*)(f->net_hdr + PICO_SIZE_GNHDR + offset))[0];
}

int64_t pico_gn_fetch_frame_timestamp(struct pico_frame *f)
{
    // Lookup table of extended headers and the offset (beginning from the extended header itself) in bytes of the timestamp inside the extended header.
    // -1 means an invalid entry, -2 means that there is no timestamp in this type of extended header.
    static const int lookup[PICO_GN_HEADER_COUNT][PICO_GN_SUBHEADER_COUNT] =
    {       
        /* ANY,      INVALID,  INVALID  */ {-2, -1, -1},
        /* BEACON,   INVALID,  INVALID  */ { 8, -1, -1},
        /* GUC,      INVALID,  INVALID  */ {12, -1, -1},
        /* GAC-circ, GAC-rect, GAC-elip */ {12, 12, 12},
        /* GBC-circ, GBC-rect, GBC-elip */ {12, 12, 12},
        /* TSB-SHB,  TSB-MHP,  INVALID  */ { 8, 12, -1}, 
        /* LS-REQ,   LS-RESP,  INVALID  */ {12, 12, -1},            
    };
    
    struct pico_gn_header *header = (struct pico_gn_header*)f->net_hdr;
    
    uint8_t header_index    = PICO_GET_GNCOMMONHDR_HEADER(header->common_header.header);
    uint8_t subheader_index = PICO_GET_GNCOMMONHDR_SUBHEADER(header->common_header.header);
    int offset = -1;
    
    if (header_index >= PICO_GN_HEADER_COUNT || subheader_index >= PICO_GN_SUBHEADER_COUNT)
        return -1;
    
    offset = lookup[header_index][subheader_index];
    
    if (offset < 0)
        return -1;
    
    return ((uint32_t*)(f->net_hdr + PICO_SIZE_GNHDR + offset))[0];
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

void pico_gn_detect_duplicate_address(struct pico_frame *f)
{
    IGNORE_PARAMETER(f);
    // TODO: implement function
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
    return a->value == b->value;
}