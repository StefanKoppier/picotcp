#include "modules/pico_geonetworking_common.c"
#include "check.h"
#include "pico_geonetworking_common.h"

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
    fail_if(pico_gn_loct_count() == 0, "Error: location table not empty after clearing it");
    
    // Check that the insertion of a NULL address doesn't get inserted.
    fail_if((entry = pico_gn_loct_add(NULL)) == NULL, "Error: inserting NULL results in the creation of an entry.");
    fail_if(pico_gn_loct_count() == 0, "Error: inserting NULL results in an entry in the location table.");
    
    // Check if an insertion of an address is successful.
    fail_if((entry = pico_gn_loct_add(address1)) != NULL &&
           (PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) == PICO_GET_GNADDR_COUNTRY_CODE(address1->value) &&
            PICO_GET_GNADDR_MANUAL(entry->address->value)       == PICO_GET_GNADDR_MANUAL(address1->value)       &&
            PICO_GET_GNADDR_MID(entry->address->value)          == PICO_GET_GNADDR_MID(address1->value)          &&
            PICO_GET_GNADDR_STATION_TYPE(entry->address->value) == PICO_GET_GNADDR_STATION_TYPE(address1->value)), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
    fail_if(pico_gn_loct_count() == 1, "Error: inserting of an entry does not result in an entry.");
    fail_if((entry->is_neighbour == 0 && 
            entry->ll_address == 0 && 
            entry->location_service_pending == 0 &&
            entry->packet_data_rate == 0 &&
            entry->position_vector == NULL &&
            entry->proto_version == 0 &&
            entry->sequence_number == 0 &&
            entry->station_type == 0 &&
            entry->timestamp == 0), "Error: inserting of an entry fails due to entry not being empty (except for the GeoNetworking address).");
    
    // Check if an insertion of a second address is successful.
    fail_if((entry = pico_gn_loct_add(address2)) != NULL &&
           (PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) == PICO_GET_GNADDR_COUNTRY_CODE(address2->value) &&
            PICO_GET_GNADDR_MANUAL(entry->address->value)       == PICO_GET_GNADDR_MANUAL(address2->value)       &&
            PICO_GET_GNADDR_MID(entry->address->value)          == PICO_GET_GNADDR_MID(address2->value)          &&
            PICO_GET_GNADDR_STATION_TYPE(entry->address->value) == PICO_GET_GNADDR_STATION_TYPE(address2->value)), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
    fail_if(pico_gn_loct_count() == 2, "Error: inserting of an entry does not result in an entry.");
    fail_if((entry->is_neighbour == 0 && 
            entry->ll_address == 0 && 
            entry->location_service_pending == 0 &&
            entry->packet_data_rate == 0 &&
            entry->position_vector == NULL &&
            entry->proto_version == 0 &&
            entry->sequence_number == 0 &&
            entry->station_type == 0 &&
            entry->timestamp == 0), "Error: inserting of an entry fails due to entry not being empty (except for the GeoNetworking address).");
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
