#include "modules/pico_geonetworking_common.c"
#include "check.h"
#include "pico_geonetworking_guc.h"

size_t pico_gn_loct_count(void)
{
    size_t count = 0;
    struct pico_tree_node *index;
    pico_tree_foreach(index, &pico_gn_loct) {
        count++;
    }
    
    return count;
}

void pico_gn_loct_clear(void)
{
    struct pico_tree_node *index;
    pico_tree_foreach_reverse(index, &pico_gn_loct) {
        pico_tree_delete(&pico_gn_loct, index);
    }
}

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
    
    // Set the GeoUnicast 
    memcpy(&extended->destination, &destination, PICO_SIZE_GNSPV);
    memcpy(&extended->source, &source, PICO_SIZE_GNLPV);
    extended->sequence_number = 123;
    
    return frame;
}

START_TEST(tc_pico_gn_guc_greedy_forward)
{
    struct pico_gn_lpv source = {
        .heading = 0,
        .sac = 0,
        .short_pv = {
            .latitude = 51436460,
            .longitude = 5469895,
            .timestamp = 1104922570ul,
            .address = 0,
        },
    };
    struct pico_gn_spv destination = {
        .address.value = 0,
        .latitude = 51436586,
        .longitude = 5469750,
        .timestamp = 1104922570ul,
    };
    
    PICO_SET_GNADDR_MID(destination.address.value, 0xFF1532516584);
    PICO_SET_GNADDR_MID(source.short_pv.address.value, 0x951753654785);
    { // Test the algorithm with no neighbours in the LocT and with the SCF flag being 0.
        pico_gn_loct_clear();
        
        fail_if(pico_gn_guc_greedy_forwarding(&destination, &pico_gn_traffic_class_default) != 0xFFFFFFFFFFFF, 
                "Error: forwarding of a packet with no items in the LocT and SCF = 0 should result in a broadcast MAC address.");
    }
    { // Test the algorithm with no neighbours in the LocT and with the SCF flag being 1.
        int result;
        struct pico_gn_traffic_class traffic_class = {
            .value = UINT8_MAX,
        };
                
        pico_gn_loct_clear();
        
        result = pico_gn_guc_greedy_forwarding(&destination, &traffic_class);
        
        fail_if(result != 0x0,
                "Error: forwarding of a packet with no items in the LocT and SCF = 1 should result in 0, with the packet in the forwarding buffer.");
    }
    { // Test the algorithm with one neighbour that is further away than the destination.
        pico_gn_loct_clear();
        
        
    }
    { // Test the algorithm with one neighbour that i closer than the destination.
        pico_gn_loct_clear();
        
        
    }
    { // Test the algorithm with one entry that is not a neighbour.
        pico_gn_loct_clear();
        
        
    }
}
END_TEST

Suite *pico_suite(void)
{
    Suite *s = suite_create("GeoNetworking GeoUnicast module");
    
    TCase *TCase_pico_gn_guc_greedy_forward = tcase_create("Unit test for GeoNetworking greedy forwarding algorithm.");
    tcase_add_test(TCase_pico_gn_guc_greedy_forward, tc_pico_gn_guc_greedy_forward);
    suite_add_tcase(s, TCase_pico_gn_guc_greedy_forward);
    
    return s;
}

int main(void)
{
    int fails;
    Suite *s = pico_suite();
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    fails = srunner_ntests_failed(sr);
    srunner_free(sr);
    return fails;
}
