#include "modules/pico_geonetworking_common.c"
#include "check.h"
#include "pico_geonetworking_common.h"
#include "pico_device.h"
#include <inttypes.h>

void pico_device_create(struct pico_device *dev, const char *name)
{
    uint32_t len = (uint32_t)strlen(name);
    int ret = 0;
    if(len > MAX_DEVICE_NAME)
        len = MAX_DEVICE_NAME;

    memcpy(dev->name, name, len);
    dev->hash = pico_hash(dev->name, len);
}


int lpv_cmp(struct pico_gn_lpv *a, struct pico_gn_lpv *b)
{
    return a->sac                == b->sac &&
           a->heading            == b->heading &&
           a->short_pv.latitude  == b->short_pv.latitude &&
           a->short_pv.longitude == b->short_pv.longitude &&
           a->short_pv.timestamp == b->short_pv.timestamp;
}

size_t pico_gn_loct_count(void)
{
    size_t count = 0;
    struct pico_tree_node *index;
    pico_tree_foreach(index, &pico_gn_loct) {
        count++;
    }
    
    return count;
}

int pico_gn_dev_link_contains(struct pico_device *device)
{
    struct pico_tree_node *index;
    pico_tree_foreach(index, &pico_gn_dev_link) {
        struct pico_gn_link *link = (struct pico_gn_link*)index->keyValue;

        if (link->dev->hash == device->hash)
            return 0;
    }
    
    return -1;
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
    { // Test for finding NULL which should result in NULL.
        // Clean setup
        pico_gn_loct_clear();
    
        fail_if(pico_gn_loct_find(NULL) != NULL, "Error: finding NULL in the LocT should return NULL.");
    }
    { // Test for finding a valid entry.
        struct pico_gn_address address = {0};
        struct pico_gn_location_table_entry *entry = NULL;
        PICO_SET_GNADDR_MID(address.value, 0x12365478);
        
        // Clean setup
        pico_gn_loct_clear();
        
        // Add the item to the LocT, do not use this result.
        pico_gn_loct_add(&address);
        
        entry = pico_gn_loct_find(&address);
        
        fail_if(!entry, "Error: finding of a valid item results in NULL.");
        fail_if(entry->address->value != address.value, "Error: found the wrong item.");
    }
    { // Test for finding an invalid entry.
        struct pico_gn_address address1 = {0};
        struct pico_gn_address address2 = {0};
        struct pico_gn_location_table_entry *entry = NULL;
        PICO_SET_GNADDR_MID(address1.value, 0x12365478);
        PICO_SET_GNADDR_MID(address2.value, 0x32156478);
        
        // Clean setup
        pico_gn_loct_clear();
        
        // Add the item to the LocT, do not use this result.
        pico_gn_loct_add(&address1);
        
        entry = pico_gn_loct_find(&address2);
        
        fail_if(entry, "Error: found an address that is not in the LocT.");
    }
    
}
END_TEST

START_TEST(tc_pico_gn_loct_update)
{
    struct pico_gn_location_table_entry *entry = NULL;
    uint8_t is_neighbour = 1;
    uint8_t station_type = (uint8_t)PEDESTRIAN;
    uint32_t timestamp = 1104922570ul;
    uint16_t sequence_number = 1523;
    struct pico_gn_address *address = PICO_ZALLOC(sizeof(address));
    struct pico_gn_lpv lpv = {
        .heading = 15621,
        .sac = 0,
        .short_pv = {
            .address = *address,
            .latitude = 145564421,
            .longitude = 1561651,
            .timestamp = 1104922570ul
        },
    };
        
    PICO_SET_GNADDR_STATION_TYPE(address->value, (uint8_t)PASSENGER_CAR);
    PICO_SET_GNADDR_MANUAL(address->value, 1);
    PICO_SET_GNADDR_MID(address->value, 0x6AB6A2ull);
    PICO_SET_GNADDR_COUNTRY_CODE(address->value, 0);
    PICO_SET_GNLPV_ACCURACY(lpv.sac, 1);
    PICO_SET_GNLPV_SPEED(lpv.sac, 100);
    
    entry = pico_gn_loct_add(address);

    fail_if(!address, "Error: could not assign memory.");
    
    int result = pico_gn_loct_update(address, &lpv, is_neighbour, sequence_number, station_type, timestamp);
    
    fail_if(result != 0, "Error: updating of LocTE resulted in an error code.");
    fail_if(entry->is_neighbour != is_neighbour, "Error: updating of the is_neighbour field failed.");
    fail_if(entry->sequence_number != sequence_number, "Error: updating of the sequence_number field failed.");
    fail_if(entry->station_type != station_type, "Error: updating of the station_type field failed.");
    fail_if(entry->proto_version != PICO_GN_PROTOCOL_VERSION, "Error: updating of the proto_version field failed.");
    fail_if(entry->timestamp != timestamp, "Error: updating of the timestamp field failed.");
    fail_if(entry->ll_address != PICO_GET_GNADDR_MID(address->value), "Error: updating of the ll_address failed.");
    fail_if(entry->location_service_pending != 0, "Error: updating of the location_service_pending field failed.");
    fail_if(lpv_cmp(&entry->position_vector, &lpv) != 1, "Error: updating of the position_vector field failed.");
}
END_TEST

START_TEST(tc_pico_gn_get_time)
{
    
}
END_TEST

START_TEST(tc_pico_gn_get_position)
{
    struct pico_gn_local_position_vector lpv = {
        .accuracy  = 0,
        .heading   = 0,
        .latitude  = 0,
        .latitude  = 0,
        .speed     = 0,
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
    struct pico_gn_location_table_entry *entry = NULL;
    struct pico_gn_address *address1 = PICO_ZALLOC(PICO_SIZE_GNADDRESS);
    PICO_SET_GNADDR_STATION_TYPE(address1->value, PASSENGER_CAR);
    PICO_SET_GNADDR_MANUAL(address1->value, 1);
    PICO_SET_GNADDR_MID(address1->value, 0x6AB6A2ul);
    PICO_SET_GNADDR_COUNTRY_CODE(address1->value, 5);
    
    struct pico_gn_address *address2 = PICO_ZALLOC(PICO_SIZE_GNADDRESS);
    PICO_SET_GNADDR_STATION_TYPE(address2->value, ROADSIDE_UNIT);
    PICO_SET_GNADDR_MANUAL(address2->value, 1);
    PICO_SET_GNADDR_MID(address2->value , 0x2D6C7Aul);
    PICO_SET_GNADDR_COUNTRY_CODE(address2->value, 5);
    
    { // Empty and check if the size of the LocT is 0.
        pico_gn_loct_clear();
        fail_if(pico_gn_loct_count() != 0, "Error: location table not empty after clearing it");
    }
    { // Check that the insertion of a NULL address doesn't get inserted.
        fail_if((entry = pico_gn_loct_add(NULL)) != NULL, "Error: inserting NULL results in the creation of an entry.");
        fail_if(pico_gn_loct_count() != 0, "Error: inserting NULL results in an entry in the location table.");
    }
    { // Check if an insertion of an address is successful.
        // Check if the GeoNetworking address of the entry gets set correctly.
        fail_if((entry = pico_gn_loct_add(address1)) == NULL, "Error: inserting an valid address in the LocT results in NULL.");
        fail_if(PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) != PICO_GET_GNADDR_COUNTRY_CODE(address1->value), "Error: inserting of a LocTE doesn't result in a correct country_code field.");
        fail_if(PICO_GET_GNADDR_MANUAL(entry->address->value)!= PICO_GET_GNADDR_MANUAL(address1->value), "Error: inserting of a LocTE doesn't result in a correct manual field.");
        fail_if(PICO_GET_GNADDR_MID(entry->address->value) != PICO_GET_GNADDR_MID(address1->value), "Error: inserting of a LocTE doesn't result in a correct MID field.");
        fail_if(PICO_GET_GNADDR_STATION_TYPE(entry->address->value) != PICO_GET_GNADDR_STATION_TYPE(address1->value), "Error: inserting of a LocTE doesn't result in a correct station_type field.");
        
        // Check if the count of the LocT get's increased.
        fail_if(pico_gn_loct_count() != 1, "Error: inserting of an entry does not result in an entry.");
        
        // Check if the rest of the entry's values are set to 0.
        fail_if(entry->is_neighbour != 0, "Error: inserting of a new LocTE should result in the field is_neighbour being 0.");
        fail_if(entry->ll_address != 0, "Error: inserting of a new LocTE should result in the field ll_address being 0.");
        fail_if(entry->location_service_pending != 0, "Error: inserting of a new LocTE should result in the field location_service_pending being 0.");
        fail_if(entry->packet_data_rate != 0, "Error: inserting of a new LocTE should result in the field packet_data_rate being 0.");
        fail_if(entry->position_vector.heading != 0, "Error: inserting of a new LocTE should result in the field position_vector.heading being 0.");
        fail_if(entry->position_vector.sac != 0, "Error: inserting of a new LocTE should result in the field position_vector.sac being 0.");
        fail_if(entry->position_vector.short_pv.latitude != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.latitude being 0.");
        fail_if(entry->position_vector.short_pv.longitude != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.longitude being 0.");
        fail_if(entry->position_vector.short_pv.timestamp != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.timestamp being 0.");
        fail_if(entry->position_vector.short_pv.address.value != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.address.value being 0.");
        fail_if(entry->proto_version != 0, "Error: inserting of a new LocTE should result in the field entry->proto_version being 0.");
        fail_if(entry->sequence_number != 0, "Error: inserting of a new LocTE should result in the field entry->sequence_number being 0.");
        fail_if(entry->station_type != 0, "Error: inserting of a new LocTE should result in the field entry->station_type being 0.");
        fail_if(entry->timestamp != 0, "Error: inserting of a new LocTE should result in the field entry->timestamp being 0.");
    }
    { // Check if an insertion of a second address is successful.
        // Check if the GeoNetworking address of the entry gets set correctly.
        fail_if((entry = pico_gn_loct_add(address2)) == NULL, "Error: inserting an valid address in the LocT results in NULL.");
        fail_if(PICO_GET_GNADDR_COUNTRY_CODE(entry->address->value) != PICO_GET_GNADDR_COUNTRY_CODE(address2->value), "Error: inserting of a LocTE doesn't result in a correct country_code field.");
        fail_if(PICO_GET_GNADDR_MANUAL(entry->address->value)!= PICO_GET_GNADDR_MANUAL(address2->value), "Error: inserting of a LocTE doesn't result in a correct manual field.");
        fail_if(PICO_GET_GNADDR_MID(entry->address->value) != PICO_GET_GNADDR_MID(address2->value), "Error: inserting of a LocTE doesn't result in a correct MID field.");
        fail_if(PICO_GET_GNADDR_STATION_TYPE(entry->address->value) != PICO_GET_GNADDR_STATION_TYPE(address2->value), "Error: inserting of a LocTE doesn't result in a correct station_type field.");
        
        // Check if the count of the LocT get's increased.
        fail_if(pico_gn_loct_count() != 2, "Error: inserting of an entry does not result in an entry.");
        
        // Check if the rest of the entry's values are set to 0.
        fail_if(entry->is_neighbour != 0, "Error: inserting of a new LocTE should result in the field is_neighbour being 0.");
        fail_if(entry->ll_address != 0, "Error: inserting of a new LocTE should result in the field ll_address being 0.");
        fail_if(entry->location_service_pending != 0, "Error: inserting of a new LocTE should result in the field location_service_pending being 0.");
        fail_if(entry->packet_data_rate != 0, "Error: inserting of a new LocTE should result in the field packet_data_rate being 0.");
        fail_if(entry->position_vector.heading != 0, "Error: inserting of a new LocTE should result in the field position_vector.heading being 0.");
        fail_if(entry->position_vector.sac != 0, "Error: inserting of a new LocTE should result in the field position_vector.sac being 0.");
        fail_if(entry->position_vector.short_pv.latitude != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.latitude being 0.");
        fail_if(entry->position_vector.short_pv.longitude != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.longitude being 0.");
        fail_if(entry->position_vector.short_pv.timestamp != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.timestamp being 0.");
        fail_if(entry->position_vector.short_pv.address.value != 0, "Error: inserting of a new LocTE should result in the field position_vector.short_pv.address.value being 0.");
        fail_if(entry->proto_version != 0, "Error: inserting of a new LocTE should result in the field entry->proto_version being 0.");
        fail_if(entry->sequence_number != 0, "Error: inserting of a new LocTE should result in the field entry->sequence_number being 0.");
        fail_if(entry->station_type != 0, "Error: inserting of a new LocTE should result in the field entry->station_type being 0.");
        fail_if(entry->timestamp != 0, "Error: inserting of a new LocTE should result in the field entry->timestamp being 0.");
    }
}
END_TEST

START_TEST(tc_pico_gn_link_add)
{
    struct pico_device *dev = PICO_ZALLOC(sizeof(struct pico_device));
    pico_device_create(dev, "mock0");
    
    { // Test for AUTO
        int result = pico_gn_link_add(dev, AUTO, 123, 123);
        
        fail_if(result != 0, "Error: managed address configuration is not implemented, this should not result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) != 0, "Error: device not added to the tree.");
    }
    /*{ // Test for MANAGED
        int result = pico_gn_link_add(dev, MANAGED, 123, 123);
        
        fail_if(result != -1, "Error: managed address configuration is not implemented, this should result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) == 0, "Error: device added to the tree, this should not happen.");
    }
    { // Test for ANONYMOUS
        int result = pico_gn_link_add(dev, ANONYMOUS, 123, 123);
        
        fail_if(result != -1, "Error: anonymous address configuration is not implemented, this should result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) == 0, "Error: device added to the tree, this should not happen.");
    }*/
    
    PICO_FREE(dev);
}
END_TEST

START_TEST(tc_pico_gn_address)
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
    Suite *s = suite_create("GeoNetworking common module");
    
    /* LOCATION TABLE TESTS */
    TCase *TCase_pico_gn_loct_find = tcase_create("Unit test for pico_gn_loct_find");
    tcase_add_test(TCase_pico_gn_loct_find, tc_pico_gn_loct_find);
    suite_add_tcase(s, TCase_pico_gn_loct_find);
    
    TCase *TCase_pico_gn_loct_add = tcase_create("Unit test for pico_gn_loct_add");
    tcase_add_test(TCase_pico_gn_loct_add, tc_pico_gn_loct_add);
    suite_add_tcase(s, TCase_pico_gn_loct_add);
    
    TCase *TCase_pico_gn_loct_update = tcase_create("Unit test for pico_gn_loct_update");
    tcase_add_test(TCase_pico_gn_loct_update, tc_pico_gn_loct_update);
    suite_add_tcase(s, TCase_pico_gn_loct_update);
    
    TCase *TCase_pico_gn_link_add = tcase_create("Unit test for adding a link between a device and a GeoNetworking address.");
    tcase_add_test(TCase_pico_gn_link_add, tc_pico_gn_link_add);
    suite_add_tcase(s, TCase_pico_gn_link_add);
    
    TCase *TCase_pico_gn_address = tcase_create("Unit test for GeoNetworking address");
    tcase_add_test(TCase_pico_gn_address, tc_pico_gn_address);
    suite_add_tcase(s, TCase_pico_gn_address);
   
    TCase *TCase_pico_gn_get_time = tcase_create("Unit test for GeoNetworking getting the system time.");
    tcase_add_test(TCase_pico_gn_get_time, tc_pico_gn_get_time);
    suite_add_tcase(s, TCase_pico_gn_get_time);
            
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
