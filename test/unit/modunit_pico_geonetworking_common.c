#include "modules/pico_geonetworking_common.c"
#include "check.h"
#include "pico_geonetworking_common.h"
#include <inttypes.h>

typedef struct pico_gn_location_table_entry locte;
typedef struct pico_gn_address address;

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

uint64_t pico_gn_mgmt_if_get_time(void)
{
    
}

struct pico_gn_local_position_vector pico_gn_mgmt_get_position_if(void)
{
    struct pico_gn_local_position_vector lpv = {
        .accuracy = 1,
        .heading = 0,
        .latitude = 5000, // This value is non-sense, but valid.
        .longitude = 4000, // This values are non-sense, but valid.
        .speed = 0,
        .timestamp = 1104922570ul,
    };
    
    return lpv;
}

START_TEST(tc_pico_gn_loct_find)
{
    /*pico_tree_empty(&pico_gn_loct);
    
    fail_if(pico_gn_loct_find(NULL) == NULL);*/
    
}
END_TEST

START_TEST(tc_pico_gn_loct_update)
{
    
}
END_TEST

START_TEST(tc_pico_gn_get_time)
{
    
}
END_TEST

START_TEST(tc_pico_gn_get_position)
{
    struct pico_gn_local_position_vector lpv = {
        .accuracy = 0,
        .heading = 0,
        .latitude = 0,
        .latitude = 0,
        .speed = 0,
        .timestamp = 0,
    };
    
    // Management interface not set, result of the get function should be -1.
    int result = pico_gn_get_position(&lpv);
    fail_if(result != -1, "Error: pico_gn_get_position with pico_gn_mgmt_interface.get_position == NULL should return -1.");
    
    pico_gn_mgmt_interface.get_position = pico_gn_mgmt_get_position_if;
    
    result = pico_gn_get_position(&lpv);
    fail_if(result != 0, "Error: pico_gn_get_position with pico_gn_mgmt_interface.get_position != NULL should return 0.");
    fail_if(lpv.accuracy != 1 || 
            lpv.heading != 0 || 
            lpv.latitude != 5000 || 
            lpv.longitude != 4000 || 
            lpv.speed != 0 || 
            lpv.timestamp != 1104922570ul,
            "Error: pico_gn_get_position doesn't return the correct function returned by the pico_gn_mgmt_interface.get_position interface.");
}
END_TEST

START_TEST(tc_pico_gn_loct_add)
{
    locte *entry = NULL;
    address *address1 = PICO_ZALLOC(sizeof(address));
    PICO_SET_GNADDR_STATION_TYPE(address1->value, PASSENGER_CAR);
    PICO_SET_GNADDR_MANUAL(address1->value, 1);
    PICO_SET_GNADDR_MID(address1->value, 0x6AB6A2);
    PICO_SET_GNADDR_COUNTRY_CODE(address1->value, 0);
    
    address *address2 = PICO_ZALLOC(sizeof(address));
    PICO_SET_GNADDR_STATION_TYPE(address2->value, ROADSIDE_UNIT);
    PICO_SET_GNADDR_MANUAL(address2->value, 1);
    PICO_SET_GNADDR_MID(address2->value , 0x2D6C7A);
    PICO_SET_GNADDR_COUNTRY_CODE(address2->value, 0);
    
    // Empty and check if the size of the LocT is 0.
    pico_gn_loct_clear();
    fail_if(pico_gn_loct_count() != 0, "Error: location table not empty after clearing it");
    
    // Check that the insertion of a NULL address doesn't get inserted.
    fail_if((entry = pico_gn_loct_add(NULL)) != NULL, "Error: inserting NULL results in the creation of an entry.");
    fail_if(pico_gn_loct_count() != 0, "Error: inserting NULL results in an entry in the location table.");
    
    // Check if an insertion of an address is successful.
    fail_if((entry = pico_gn_loct_add(address1)) != NULL &&
           !(PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) == PICO_GET_GNADDR_COUNTRY_CODE(address1->value) &&
             PICO_GET_GNADDR_MANUAL(entry->address->value)       == PICO_GET_GNADDR_MANUAL(address1->value)       &&
             PICO_GET_GNADDR_MID(entry->address->value)          == PICO_GET_GNADDR_MID(address1->value)          &&
             PICO_GET_GNADDR_STATION_TYPE(entry->address->value) == PICO_GET_GNADDR_STATION_TYPE(address1->value)), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
    fail_if(pico_gn_loct_count() != 1, "Error: inserting of an entry does not result in an entry.");
    fail_if((entry->is_neighbour             != 0 || 
             entry->ll_address               != 0 || 
             entry->location_service_pending != 0 ||
             entry->packet_data_rate         != 0 ||
             entry->position_vector          != NULL ||
             entry->proto_version            != 0 ||
             entry->sequence_number          != 0 ||
             entry->station_type             != 0 ||
             entry->timestamp                != 0), "Error: inserting of an entry fails due to entry not being empty (except for the GeoNetworking address).");
    
    // Check if an insertion of a second address is successful.
    fail_if((entry = pico_gn_loct_add(address2)) != NULL &&
           !(PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) == PICO_GET_GNADDR_COUNTRY_CODE(address2->value) &&
             PICO_GET_GNADDR_MANUAL(entry->address->value)       == PICO_GET_GNADDR_MANUAL(address2->value)       &&
             PICO_GET_GNADDR_MID(entry->address->value)          == PICO_GET_GNADDR_MID(address2->value)          &&
             PICO_GET_GNADDR_STATION_TYPE(entry->address->value) == PICO_GET_GNADDR_STATION_TYPE(address2->value)), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
    fail_if(pico_gn_loct_count() != 2, "Error: inserting of an entry does not result in an entry.");
    fail_if(entry->is_neighbour             != 0 || 
            entry->ll_address               != 0 || 
            entry->location_service_pending != 0 ||
            entry->packet_data_rate         != 0 ||
            entry->position_vector          != NULL ||
            entry->proto_version            != 0 ||
            entry->sequence_number          != 0 ||
            entry->station_type             != 0 ||
            entry->timestamp                != 0, "Error: inserting of an entry fails due to entry not being empty (except for the GeoNetworking address).");
}
END_TEST

START_TEST(pico_gn_address)
{
    { // Test GET functions from buffers assignment.
        struct pico_gn_address addr = {
            .value = 0xbc9a7856341221bc,
        };
        
        uint64_t mid = PICO_GET_GNADDR_MID(addr.value);
        fail_if(mid != 0xbc9a78563412, "PICO_GET_GNADDR_MID failed. Expected %"PRIx64", got %"PRIx64, 0xbc9a78563412, mid);
        uint16_t country_code = PICO_GET_GNADDR_COUNTRY_CODE(addr.value);
        fail_if(country_code != 33, "PICO_GET_GNADDR_COUNTRY_CODE failed. Expected %hu, got %hu", 33, country_code);
        uint8_t manual = PICO_GET_GNADDR_MANUAL(addr.value);
        fail_if(manual != 1, "PICO_GET_GNADDR_MANUAL failed. Expected %u, got %u", 1, manual);
        uint16_t station_type = PICO_GET_GNADDR_STATION_TYPE(addr.value);
        fail_if(station_type != 15, "PICO_GET_GNADDR_STATION_TYPE failed. Expected %hu got %hu.", 15, station_type);
    }
    { // Test GET functions from SET assignments.
        struct pico_gn_address *addr = PICO_ZALLOC(PICO_SIZE_GNADDRESS);
        addr->value = 0;
        
        uint8_t  manual = 0x00;
        uint8_t  station_type = 0xB;
        uint16_t country_code = 0x326;
        uint64_t mid = 0x0CFCAF36F200;

        PICO_SET_GNADDR_MANUAL(addr->value, manual);
        fail_if(PICO_GET_GNADDR_MANUAL(addr->value) != manual, "PICO_SET_GNADDR_MANUAL failed. Expected %u, got %u", manual, PICO_GET_GNADDR_MANUAL(addr->value));

        PICO_SET_GNADDR_STATION_TYPE(addr->value, station_type);
        fail_if(PICO_GET_GNADDR_STATION_TYPE(addr->value) != station_type, "PICO_SET_GNADDR_STATION_TYPE failed. Expected %hu got %hu.", station_type, PICO_GET_GNADDR_STATION_TYPE(addr->value));

        PICO_SET_GNADDR_COUNTRY_CODE(addr->value, country_code);
        fail_if (PICO_GET_GNADDR_COUNTRY_CODE(addr->value) != country_code, "PICO_SET_GNADDR_COUNTRY_CODE failed. Expected %hu, got %hu", country_code, PICO_GET_GNADDR_COUNTRY_CODE(addr->value));

        PICO_SET_GNADDR_MID(addr->value, mid);
        fail_if (PICO_GET_GNADDR_MID(addr->value) != mid, "PICO_SET_GNADDR_MID failed. Expected %"PRIx64", got %"PRIx64, mid, PICO_GET_GNADDR_MID(addr->value));
    }
}
END_TEST

Suite *pico_suite(void)
{
    Suite *s = suite_create("GeoNetworking module");
    
    /* LOCATION TABLE TESTS */
    TCase *TCase_pico_gn_loct_find = tcase_create("Unit test for pico_gn_loct_find");
    tcase_add_test(TCase_pico_gn_loct_find, tc_pico_gn_loct_find);
    suite_add_tcase(s, TCase_pico_gn_loct_find);
    
    TCase *TCase_pico_gn_loct_update = tcase_create("Unit test for pico_gn_loct_update");
    tcase_add_test(TCase_pico_gn_loct_update, tc_pico_gn_loct_update);
    suite_add_tcase(s, TCase_pico_gn_loct_update);
    
    TCase *TCase_pico_gn_loct_add = tcase_create("Unit test for pico_gn_loct_add");
    tcase_add_test(TCase_pico_gn_loct_add, tc_pico_gn_loct_add);
    suite_add_tcase(s, TCase_pico_gn_loct_add);
    
    TCase *TCase_pico_gn_address = tcase_create("Unit test for GeoNetworking address");
    tcase_add_test(TCase_pico_gn_address, pico_gn_address);
    suite_add_tcase(s, TCase_pico_gn_address);
    
    TCase *TCase_pico_gn_get_position = tcase_create("Unit test for GeoNetworking getting the Local Position.");
    tcase_add_test(TCase_pico_gn_get_position, tc_pico_gn_get_position);
    suite_add_tcase(s, TCase_pico_gn_get_position);
        
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
