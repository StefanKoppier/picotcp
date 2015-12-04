#include "pico_geonetworking_guc.h"
#include "pico_tree.h"
#include "pico_geonetworking_common.h"
#include "pico_frame.h"
#include "pico_geonetworking_management.h"
#include "pico_protocol.h"
#include "pico_eth.h"

int pico_gn_guc_send(struct pico_gn_address *destination)
{
    next_alloc_header_type = &guc_header_type;
    struct pico_frame *f = pico_proto_geonetworking.alloc(&pico_proto_geonetworking, 0);
    return pico_proto_geonetworking.push(&pico_proto_geonetworking, f);
}

const struct pico_gn_header_info guc_header_type = {
    .header     = 2,
    .subheader  = 0,
    .size       = PICO_SIZE_GUCHDR,
    .offsets = {
        .timestamp       = 12,
        .sequence_number = 0,
        .source_address  = 4,
    },
    .push       = pico_gn_guc_push,
    .in         = pico_gn_process_guc_in,
    .out        = pico_gn_process_guc_out,
    .alloc      = pico_gn_guc_alloc,
};

int pico_gn_guc_push(struct pico_frame *f)
{
    // TODO: implement function
    return -1;
}

int pico_gn_process_guc_in(struct pico_frame *f)
{
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_address *dest_addr = &extended->destination.address;
    
    // Check if the destination address of the packet is the local address
    struct pico_gn_link *entry;
    int found_in_devices = 0;
    
    dbg("Received address: %lx\n", dest_addr->value);
        
    // ACT IF A MID FIELD OF ALL 1's IS FOR THIS GEOADHOC ROUTER.
    // THIS IS NOT PART OF THE GEONETWORKING STANDARD AND MUST BE REMOVED WHEN TWHE PROTOCOL IS DEPLOYED.
    // THIS IS A POTENTIAL UNDEFINED BEHAVIOUR CAUSE IF THE DEST ADDR IS ACTUALY ALL 1's BUT NOT DETERMINDED FOR THIS GEOADHOC ROUTER
    if (PICO_GET_GNADDR_MID(dest_addr->value) == 0xbc9a78563412)
    {
        found_in_devices = 1;
    }
    else
    {
        struct pico_tree_node *index;
        
        pico_tree_foreach(index, &pico_gn_dev_link) {
            entry = (struct pico_gn_link*)index->keyValue;

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
    struct pico_gn_guc_header           *extended     = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_header               *header       = (struct pico_gn_header*)f->net_hdr;
    struct pico_gn_lpv                  *source       = &extended->source;
    struct pico_gn_address              *source_addr  = &extended->source.short_pv.address;
    struct pico_gn_location_table_entry *locte        = pico_gn_loct_find(source_addr);
    int                                  is_duplicate = pico_gn_detect_duplicate_sntst_packet(f);
    struct pico_tree_node               *index        = NULL;
    
    // Check if this packet is a duplicate.
    switch (is_duplicate)
    {
    case 0: break; // Not a duplicate, continue
    case 1: // A duplicate, exit quietly.
    case -1: // Failure, exit with an error code.
        pico_frame_discard(f);
        dbg("Received a duplicate, discard the packet.\n");
        // TODO: set error code if result == -1
        return is_duplicate;
    }

    // Check the local address for duplicate addresses, if that's so, update it.
    pico_tree_foreach(index, &pico_gn_dev_link) {
        struct pico_gn_link *entry = (struct pico_gn_link*)index->keyValue;

        if (PICO_GET_GNADDR_MID(entry->address.value) == PICO_GET_GNADDR_MID(source_addr->value))
        {
            pico_gn_create_address_auto(&entry->address, PICO_GET_GNADDR_STATION_TYPE(entry->address.value), PICO_GET_GNADDR_COUNTRY_CODE(entry->address.value));
            dbg("The address is a duplicate. Refreshing the address.\n");
            break;
        }
    }
    
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
    case BTP_A:
        pico_transport_receive(f, PICO_PROTO_BTP_A);
        return 0;
    case BTP_B:
        pico_transport_receive(f, PICO_PROTO_BTP_B);
        return 0;
    case GN6ASL:
        dbg("GN6ASL packet received, but GN6ASL is not implemented. The packet shall be discarded.\n");
        pico_frame_discard(f);
        return 0;
    case CM_ANY: // GeoNetworking not specifies what to do with an 'any' transport layer protocol, throw the packet away.
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

int pico_gn_process_guc_out(struct pico_frame *f)
{
    // TODO: implement function.
    return -1;
}

struct pico_frame *pico_gn_guc_alloc(uint16_t size)
{
    struct pico_frame *f = pico_frame_alloc(PICO_SIZE_ETHHDR + PICO_SIZE_GNHDR + PICO_SIZE_GUCHDR + size);
    
    if (!f)
        pico_err = PICO_ERR_ENOMEM;
                
    return f;
}

uint64_t pico_gn_guc_greedy_forwarding(const struct pico_gn_spv *dest, const struct pico_gn_traffic_class *traffic_class)
{
    struct pico_gn_local_position_vector lpv            = {0};
    struct pico_tree_node               *index          = NULL;
    double                               distance_best  = 0;
    struct pico_gn_location_table_entry *hop_best       = NULL;
    uint64_t                             mid_best       = 0;
    uint8_t                              has_neighbours = 0;
    
    // Get the Local Position Vector.
    pico_gn_get_position(&lpv);
    
    distance_best = pico_gn_calculate_distance(dest->latitude, dest->longitude, lpv.latitude, lpv.longitude);
    
    // Find the entry that is the best choice for forwarding.
    pico_tree_foreach(index, &pico_gn_loct) {
        struct pico_gn_location_table_entry *entry = (struct pico_gn_location_table_entry*)index->keyValue;
        
        // Check if the entry is a neighbour.
        if (entry->is_neighbour)
        {
            double distance_current = pico_gn_calculate_distance(
                    dest->latitude, dest->longitude,
                    entry->position_vector.short_pv.latitude, entry->position_vector.short_pv.longitude);
            
            has_neighbours = 1;
            
            // Check is this entry is a better choice than the previous.
            if (distance_current < distance_best)
            {
                distance_best = distance_current;
                hop_best = entry;
            }
        }
    }
    
    // Check if the destination is closer than the nearest neighbour, if so forward to the destination.
    if (distance_best < pico_gn_calculate_distance(dest->latitude, dest->longitude, lpv.latitude, lpv.longitude))
    {
        mid_best = PICO_GET_GNADDR_MID(hop_best->address->value);
    }
    else // There is a neighbour closer than the destination, forward to this.
    {
        if (!has_neighbours && PICO_GET_GNTRAFFIC_CLASS_SCF((traffic_class)) == 1)
        {
            // TODO: Add this frame to the forwarding packet buffer
            mid_best = 0;
        }
        else
        {
            // This packet cannot be forwarded or stored, try to broadcast it.
            mid_best = 0xFFFFFFFFFFFF;
        }
    }
    
    return mid_best;
}
