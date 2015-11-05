#include "pico_geonetworking_guc.h"
#include "pico_tree.h"

const struct pico_gn_header_info guc_header_type = {
    .header     = 2,
    .subheader  = 0,
    .size       = PICO_SIZE_GUCHDR,
    .offsets = {
        .timestamp       = 12,
        .sequence_number = 0,
        .source_address  = 4,
    },
    .in         = pico_gn_process_guc_in,
    .out        = pico_gn_process_guc_out,
    .alloc      = pico_gn_guc_alloc,
};

int pico_gn_process_guc_in(struct pico_frame *f)
{
    struct pico_gn_guc_header *extended = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
    struct pico_gn_address *dest_addr = &extended->destination.address;
    
    // Check if the destination address of the packet is the local address
    struct pico_gn_link *entry;
    int found_in_devices = 0;
    
    dbg("Received a GeoNetworking address: \n");
    dbg("manual: %d\n", PICO_GET_GNADDR_MANUAL(dest_addr->value));
    dbg("station_type: %d\n", PICO_GET_GNADDR_STATION_TYPE(dest_addr->value));
    dbg("country_code: %d\n", PICO_GET_GNADDR_COUNTRY_CODE(dest_addr->value));
    dbg("mid: %016llX\n\n", PICO_GET_GNADDR_MID(dest_addr->value));
      
    
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
        struct pico_tree_node *index;
        
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
    // TODO: implement function.
    return NULL;
}