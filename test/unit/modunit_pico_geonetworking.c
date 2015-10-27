#include "modules/pico_geonetworking.c"
#include "check.h"

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
    /*locte *entry = NULL;
    address *address1 = PICO_ZALLOC(sizeof(address));
    address1->station_type = PICO_GN_STATION_TYPE_PASSENGER_CAR;
    address1->manual = 1;
    address1->mid = 0x6AB6A2;
    address1->country_code = 0;
    
    address *address2 = PICO_ZALLOC(sizeof(address));
    address2->station_type = PICO_GN_STATION_TYPE_ROADSIDE_UNIT;
    address2->manual = 1;
    address2->mid = 0x2D6C7A;
    address2->country_code = 0;
    
    // Empty and check if the size of the LocT is 0.
    pico_gn_loct_clear();
    fail_if(pico_gn_loct_count() == 0, "Error: location table not empty after clearing it");
    
    // Check that the insertion of a NULL address doesn't get inserted.
    fail_if((entry = pico_gn_loct_add(NULL)) == NULL, "Error: inserting NULL results in the creation of an entry.");
    fail_if(pico_gn_loct_count() == 0, "Error: inserting NULL results in an entry in the location table.");
    
    // Check if an insertion of an address is successful.
    fail_if((entry = pico_gn_loct_add(address1)) != NULL &&
           (entry->address->country_code == address1->country_code &&
            entry->address->manual == address1->manual &&
            entry->address->mid == address1->mid &&
            entry->address->station_type == address1->station_type ), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
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
           (entry->address->country_code == address2->country_code &&
            entry->address->manual == address2->manual &&
            entry->address->mid == address2->mid &&
            entry->address->station_type == address2->station_type ), "Error: inserting an entry fails due to returning of NULL or incorrectly inserting the GeoNetworking address.");
    fail_if(pico_gn_loct_count() == 2, "Error: inserting of an entry does not result in an entry.");
    fail_if((entry->is_neighbour == 0 && 
            entry->ll_address == 0 && 
            entry->location_service_pending == 0 &&
            entry->packet_data_rate == 0 &&
            entry->position_vector == NULL &&
            entry->proto_version == 0 &&
            entry->sequence_number == 0 &&
            entry->station_type == 0 &&
            entry->timestamp == 0), "Error: inserting of an entry fails due to entry not being empty (except for the GeoNetworking address).");*/
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
