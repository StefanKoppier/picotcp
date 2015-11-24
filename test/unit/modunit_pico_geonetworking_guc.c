#include "modules/pico_geonetworking_common.c"
#include "modunit_pico_geonetworking_shared.c"
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
