#include "modules/pico_geonetworking_common.c"
#include "modunit_pico_geonetworking_shared.c"
#include "check.h"
#include "pico_geonetworking_common.h"
#include "pico_device.h"
#include "pico_frame.h"
#include <inttypes.h>

#define IN_DELTA(x, y, d) (((x + d) <= y) && ((x - d) >= y))

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

int pico_gn_dev_link_count(void)
{
    struct pico_tree_node *index;
    int count = 0;
    
    pico_tree_foreach(index, &pico_gn_dev_link) {
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

void pico_gn_dev_link_clear(void)
{
    struct pico_tree_node *index, *_tmp;
    struct pico_gn_dev_link *entry;
    pico_tree_foreach_safe(index, &pico_gn_dev_link, _tmp) {
        entry = (struct pico_gn_dev_link*)index->keyValue;
        
        pico_tree_delete(&pico_gn_dev_link, entry);
        PICO_FREE(entry);
    }
}

void pico_gn_loct_clear(void)
{
    struct pico_tree_node *index, *_tmp;
    struct pico_gn_location_table_entry *entry;
    pico_tree_foreach_safe(index, &pico_gn_loct, _tmp) {
        entry = (struct pico_gn_location_table_entry*)index->keyValue;
        
        pico_tree_delete(&pico_gn_loct, entry);
        PICO_FREE(entry);
    }
}

uint64_t pico_gn_mgmt_if_get_time(void)
{
    return 1448292875136ull;
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
        struct pico_gn_address address = (struct pico_gn_address){0};
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
        struct pico_gn_address address1 = (struct pico_gn_address){0};
        struct pico_gn_address address2 = (struct pico_gn_address){0};
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
    pico_gn_mgmt_interface.get_time = NULL;
    { // Management interface not set, result of the get function should be -1.
        uint64_t *time = PICO_ZALLOC(sizeof(uint64_t));
        pico_gn_mgmt_interface.get_time = NULL;
        int result = pico_gn_get_current_time(time);
        
        fail_if(result != -1, "Error: pico_gn_get_current_time with pico_gn_mgmt_interface.get_current_time == NULL should return -1.");
    }
    { // Management interface set, result should be 0.
        uint64_t *time = PICO_ZALLOC(sizeof(uint64_t));
        pico_gn_mgmt_interface.get_time = pico_gn_mgmt_if_get_time;
        int result = pico_gn_get_current_time(time);
        
        fail_if(result != 0, "Error: pico_gn_get_current_time with pico_gn_mgmt_interface.get_current_time set should return 0.");
        fail_if(*time == 0, "Error: pico_gn_get_current_time does not results in a time.");
    }
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
    
    pico_gn_mgmt_interface.get_position = NULL;
    
    // Management interface not set, result of the get function should be -1.
    int result = pico_gn_get_position(&lpv);
    fail_if(result != -1, "Error: pico_gn_get_position with pico_gn_mgmt_interface.get_position == NULL should return -1.");
    
    pico_gn_mgmt_interface.get_position = pico_gn_mgmt_get_position_if;
    
    result = pico_gn_get_position(&lpv);
    fail_if(result != 0, "Error: pico_gn_get_position with pico_gn_mgmt_interface.get_position set should return 0.");
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
    { // Test for AUTO
        struct pico_device *dev = PICO_ZALLOC(sizeof(struct pico_device));
        pico_device_create(dev, "mock0");
        int result = pico_gn_link_add(dev, AUTO, 123, 123);
      
        fail_if(result != 0, "Error: managed address configuration is not implemented, this should not result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) != 0, "Error: device not added to the tree.");
        
        pico_gn_dev_link_clear();
        fail_if(pico_gn_dev_link_count() != 0, "Error: unit test function pico_gn_dev_link_clear isn't functioning.");
        PICO_FREE(dev);
    }
    { // Test for MANAGED
        struct pico_device *dev = PICO_ZALLOC(sizeof(struct pico_device));
        pico_device_create(dev, "mock1");
        int result = pico_gn_link_add(dev, MANAGED, 123, 123);
        
        fail_if(result != -1, "Error: managed address configuration is not implemented, this should result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) == 0, "Error: device added to the tree, this should not happen.");
        
        pico_gn_dev_link_clear();
    }
    { // Test for ANONYMOUS
        struct pico_device *dev = PICO_ZALLOC(sizeof(struct pico_device));
        pico_device_create(dev, "mock2");
        int result = pico_gn_link_add(dev, ANONYMOUS, 123, 123);
        
        fail_if(result != -1, "Error: anonymous address configuration is not implemented, this should result in an error code.");
        fail_if(pico_gn_dev_link_contains(dev) == 0, "Error: device added to the tree, this should not happen.");
        
        pico_gn_dev_link_clear();
    }
}
END_TEST

START_TEST(tc_pico_gn_link_compare)
{
    { // Test with an address with a MID field that is smaller than the other, this should result in -1
        struct pico_gn_link a = (struct pico_gn_link){0};
        struct pico_gn_link b = (struct pico_gn_link){0};
        
        PICO_SET_GNADDR_MID(a.address.value, 0x222222222222ull);
        PICO_SET_GNADDR_MID(b.address.value, 0x111111111111ull);
        
        fail_if(pico_gn_link_compare(&a, &b) != -1, "Error: a > b should result in -1.");
    }
    { // Test with an address with a MID field that is bigger than the other, this should result in 1.
        struct pico_gn_link a = (struct pico_gn_link){0};
        struct pico_gn_link b = (struct pico_gn_link){0};
        
        PICO_SET_GNADDR_MID(a.address.value, 0x111111111111ull);
        PICO_SET_GNADDR_MID(b.address.value, 0x222222222222ull);
        
        fail_if(pico_gn_link_compare(&a, &b) != 1, "Error: a < b should result in 1.");
        
    }
    { // Test with two addresses that have the same MID field, this should result in 0.
        struct pico_gn_link a = (struct pico_gn_link){0};
        struct pico_gn_link b = (struct pico_gn_link){0};
        
        PICO_SET_GNADDR_MID(a.address.value, 0x111111111111ull);
        PICO_SET_GNADDR_MID(b.address.value, 0x111111111111ull);
        
        fail_if(pico_gn_link_compare(&a, &b) != 0, "Error: a == b should result in 0.");
        
    }
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

START_TEST(tc_pico_gn_fetch_frame_source_address)
{
    { // GeoUnicast frame
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
        
        PICO_SET_GNADDR_MID(source.short_pv.address.value, 0x15615FEull);
        PICO_SET_GNADDR_COUNTRY_CODE(source.short_pv.address.value, 12);
        PICO_SET_GNADDR_MANUAL(source.short_pv.address.value, 1);
        PICO_SET_GNADDR_STATION_TYPE(source.short_pv.address.value, (uint8_t)PEDESTRIAN);
        
        struct pico_gn_spv destination = {
            .address = 0,
            .latitude = 51436586,
            .longitude = 5469750,
            .timestamp = 1104922570ul,
        };
            
        struct pico_frame *f = pico_gn_create_guc_packet(source, destination);
        struct pico_gn_address *address = pico_gn_fetch_frame_source_address(f);
        
        fail_if(address->value != source.short_pv.address.value, "Error: incorrect address returned.");
     }
}
END_TEST

START_TEST(tc_pico_gn_fetch_frame_sequence_number)
{
    { // GeoUnicast frame
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
        
        PICO_SET_GNADDR_MID(source.short_pv.address.value, 0x15615FEull);
        PICO_SET_GNADDR_COUNTRY_CODE(source.short_pv.address.value, 12);
        PICO_SET_GNADDR_MANUAL(source.short_pv.address.value, 1);
        PICO_SET_GNADDR_STATION_TYPE(source.short_pv.address.value, (uint8_t)PEDESTRIAN);
        
        struct pico_gn_spv destination = {
            .address = 0,
            .latitude = 51436586,
            .longitude = 5469750,
            .timestamp = 1104922570ul,
        };
            
        struct pico_frame *f = pico_gn_create_guc_packet(source, destination);
        struct pico_gn_address *address = pico_gn_fetch_frame_source_address(f);
        struct pico_gn_guc_header *header = (struct pico_gn_guc_header*)(f->net_hdr + PICO_SIZE_GNHDR);
        uint16_t sequence_number = 123654;
        header->sequence_number = sequence_number;
        
        fail_if(pico_gn_fetch_frame_sequence_number(f) != sequence_number, "Error: incorrect sequence_number returned.");
     }
}
END_TEST

START_TEST(tc_pico_gn_fetch_frame_timestamp)
{
    { // GeoUnicast frame test
        uint32_t timestamp = 1104922570ul;
        struct pico_gn_lpv source = {
            .heading = 0,
            .sac = 0,
            .short_pv = {
                .latitude = 51436460,
                .longitude = 5469895,
                .timestamp = timestamp,
                .address = 0,
            },
        };
        
        struct pico_gn_spv destination = {
            .address = 0,
            .latitude = 51436586,
            .longitude = 5469750,
            .timestamp = 0,
        };
            
        struct pico_frame *f = pico_gn_create_guc_packet(source, destination);
        
        fail_if(pico_gn_fetch_frame_timestamp(f) != timestamp, "Error: incorrect timestamp returned.");
     }
}
END_TEST

START_TEST(tc_pico_gn_fetch_loct_timestamp)
{
    pico_gn_loct_clear();
    
    struct pico_gn_location_table_entry *entry1 = NULL;
    struct pico_gn_address address1 = (struct pico_gn_address){0};
    uint32_t timestamp1 = 1448292875136ul;
    PICO_SET_GNADDR_MID(address1.value, 0x123654987ull);
    PICO_SET_GNADDR_COUNTRY_CODE(address1.value, 12);
    entry1 = pico_gn_loct_add(&address1);
    entry1->timestamp = timestamp1;
    
    struct pico_gn_location_table_entry *entry2 = NULL;
    struct pico_gn_address address2 = (struct pico_gn_address){0};
    uint32_t timestamp2 = 1448292875951ul;
    PICO_SET_GNADDR_MID(address2.value, 0x12365297ull);
    PICO_SET_GNADDR_COUNTRY_CODE(address2.value, 4);
    entry2 = pico_gn_loct_add(&address2);
    entry2->timestamp = timestamp2;
    
    { // Test with NULL, this should result in -1
        fail_if(pico_gn_fetch_loct_timestamp(NULL) != -1, "Error: finding the LocTE timestamp of NULL should result in -1.");
    }
    { // Test entry1, this should result in 1448292875136ul
        fail_if(pico_gn_fetch_loct_timestamp(&address1) != timestamp1, "Error: finding the LocTE timestamp of address1 should result in 1448292875136.");
    }
    { // Test entry2, this should result in 1448292875951ul
        fail_if(pico_gn_fetch_loct_timestamp(&address2) != timestamp2, "Error: finding the LocTE timestamp of address1 should result in 1448292875951.");
    }
}
END_TEST

START_TEST(tc_pico_gn_fetch_loct_sequence_number)
{
    pico_gn_loct_clear();
    
    struct pico_gn_location_table_entry *entry1 = NULL;
    struct pico_gn_address address1 = (struct pico_gn_address){0};
    uint32_t sequence_number1 = 152;
    PICO_SET_GNADDR_MID(address1.value, 0x123654987ull);
    PICO_SET_GNADDR_COUNTRY_CODE(address1.value, 12);
    entry1 = pico_gn_loct_add(&address1);
    entry1->sequence_number = sequence_number1;
    
    struct pico_gn_location_table_entry *entry2 = NULL;
    struct pico_gn_address address2 = (struct pico_gn_address){0};
    uint32_t sequence_number2 = 15;
    PICO_SET_GNADDR_MID(address2.value, 0x12365297ull);
    PICO_SET_GNADDR_COUNTRY_CODE(address2.value, 4);
    entry2 = pico_gn_loct_add(&address2);
    entry2->sequence_number = sequence_number2;
    
    { // Test with NULL, this should result in -1
        fail_if(pico_gn_fetch_loct_sequence_number(NULL) != -1, "Error: finding the LocTE sequence_number of NULL should result in -1.");
    }
    { // Test entry1, this should result in 152
        fail_if(pico_gn_fetch_loct_sequence_number(&address1) != sequence_number1, "Error: finding the LocTE sequence_number of address1 should result in 152.");
    }
    { // Test entry2, this should result in 15
        fail_if(pico_gn_fetch_loct_sequence_number(&address2) != sequence_number2, "Error: finding the LocTE sequence_number of address1 should result in 15.");
    }
}
END_TEST

START_TEST(tc_pico_gn_get_sequence_number)
{
    uint32_t i;
    for (i = 1; i <= UINT16_MAX + 1; ++i)
    {
        uint16_t sn = pico_gn_get_next_sequence_number();
        
        if (i == 1)
            fail_if(sn != 1, "Error: the first sequence number should be 1.");
        else if (i == 2)
            fail_if(sn != 2, "Error: the second sequence number should be 2.");
        if(i == (UINT16_MAX + 1))
            fail_if(sn != 1, "Error: the sequence number overflow does not return to 1.");
    }
}
END_TEST

START_TEST(tc_pico_gn_find_extended_header_length)
{
    int64_t result = -1;
    
    { // Test for NULL, should return -1
        result = pico_gn_find_extended_header_length(NULL);
        
        fail_if(result != -1, "Error: getting the extended header length of NULL should result in -1.");
    }
    { // Test for an invalid header and subheader type, should return -1
        struct pico_gn_header header = (struct pico_gn_header){0};
        PICO_SET_GNCOMMONHDR_HEADER(header.common_header.header, 3);
        PICO_SET_GNCOMMONHDR_SUBHEADER(header.common_header.header, 3);

        result = pico_gn_find_extended_header_length(&header);
        
        fail_if(result != -1, "Error: getting the extended header length of an invalid extended header should result in -1.");
    }
    { // Test for GeoUnicast, should return 48.
        struct pico_gn_header header = (struct pico_gn_header){0};
        PICO_SET_GNCOMMONHDR_HEADER(header.common_header.header, guc_header_type.header);
        PICO_SET_GNCOMMONHDR_SUBHEADER(header.common_header.header, guc_header_type.subheader);

        result = pico_gn_find_extended_header_length(&header);

        fail_if(result != 48, "Error: the size of the GeoUnicast header should be 48.");
    }
}
END_TEST

START_TEST(tc_pico_gn_get_header_info)
{
    struct pico_gn_header_info *result = NULL;
    
    { // Test with a NULL header, this should result in header_info_invalid
        result = pico_gn_get_header_info(NULL);
        
        fail_if(result != &header_info_invalid, "Error: getting the header info of NULL results in an header type that is not header_info_invalid.");
    }
    { // Test with a invalid header and subheader, should result in header_info_invalid
        struct pico_gn_header header = (struct pico_gn_header){0};
        PICO_SET_GNCOMMONHDR_HEADER(header.common_header.header, 3);
        PICO_SET_GNCOMMONHDR_SUBHEADER(header.common_header.header, 3);

        result = pico_gn_get_header_info(&header);
            
        fail_if(result != &header_info_invalid, "Error: getting the header info of header with an invalid header and subheader does not result in the header type for invalid.");
    }
    { // Test for a GeoUnicast
        { // Test with a valid subheader, should result in guc_header_type
            struct pico_gn_header header = (struct pico_gn_header){0};
            PICO_SET_GNCOMMONHDR_HEADER(header.common_header.header, guc_header_type.header);
            PICO_SET_GNCOMMONHDR_SUBHEADER(header.common_header.header, guc_header_type.subheader);
            
            result = pico_gn_get_header_info(&header);
            
            fail_if(result != &guc_header_type, "Error: getting the header info of a GeoUnicast header type does not result in the header type for GeoUnicast.");
        }
    }
}
END_TEST

START_TEST(tc_pico_gn_detect_duplicate_sntst_packet)
{
    struct pico_gn_address source_addr = (struct pico_gn_address){0};
    struct pico_gn_spv destination = (struct pico_gn_spv){0};
    PICO_SET_GNADDR_MID(source_addr.value, 0x111111111111ull);
    
    const uint8_t duplicate = 1;
    const uint8_t not_duplicate = 0;
    
    pico_gn_loct_clear();

    { // Test the scenario where there is no entry in the LocT, this should result in 0 (this must be the newest).
        struct pico_gn_lpv source = {
            .sac = 0,
            .heading = 0,
            .short_pv.latitude = 0,
            .short_pv.longitude = 0,
            .short_pv.address = source_addr,
            .short_pv.timestamp = 1104922570ul,
        };
        
        uint16_t sequence_number = 15;
        struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, sequence_number);
        
        fail_if(pico_gn_detect_duplicate_sntst_packet(f) != not_duplicate, "Error: a packet received with no LocTE of this source must be the newest.");
    }
    { // Tests the scenarios where the timestamp is more recent in the one in the LocT.
        { // Test with a newer sequence number, this should result in 0.
            // Nice to have: implement scenario. This is not really necessary because the protocol 
            // doesn't look at the sequence number if the timestamp is newer.
        }
        { // Test with the same sequence number as the last, this should result in 0.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922580ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 3);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922570ul;
            entry->sequence_number = 2;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != not_duplicate, "Error: a packet received with a more recent timestamp and a higher sequence number is newer than the entry thus should result in 0.");
            pico_gn_loct_clear();
        }
        { // Test with an older sequence number as the last, this should result in 0.
            // Nice to have: implement scenario. This is not really necessary because the protocol 
            // doesn't look at the sequence number if the timestamp is newer.
        }
    }
    { // Test the scenarios where the timestamp is equal to the one in the LocT.
        { // Test with a newer sequence number, this should result in 0.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 3);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922570ul;
            entry->sequence_number = 2;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != not_duplicate, "Error: a packet received with a timestamp equal to the LocTE but the higher sequence number is newer than the entry thus should result in 0.");
        }
        { // Test with a sequence number equal to the last, this should result in 1.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 3);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922570ul;
            entry->sequence_number = 3;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != duplicate, "Error: a packet received with the timestamp and sequence number equal to the LocTE.");
        }
        { // Test with a sequence number lower than the last, this should result in 1.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 2);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922570ul;
            entry->sequence_number = 3;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != duplicate, "Error: a packet received with a timestamp equal to the LocTE but the higher sequence number is newer than the entry thus should result in 1.");
        }
    }
    { // Test the scenarios where that timestamp is older than the one in the LocT.
        { // Test with a newer sequence number, this should result in 1.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 2);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922580ul;
            entry->sequence_number = 3;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != duplicate, "Error: a packet received with a timestamp older than the one in the LocTE but a higher sequence number is newer than the entry which should result in 1.");
        }
        { // Test with a sequence number equal to the last, this should result in 1.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 2);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922580ul;
            entry->sequence_number = 2;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != duplicate, "Error: a packet received with a timestamp older than the one in the LocTE but the higher sequence number is newer than the entry thus should result in 1.");

        }
        { // Test with a sequence number lower than the last, this should result in 1.
            struct pico_gn_lpv source = {
                .sac = 0,
                .heading = 0,
                .short_pv.latitude = 0,
                .short_pv.longitude = 0,
                .short_pv.address = source_addr,
                .short_pv.timestamp = 1104922570ul,
            };

            struct pico_frame *f = pico_gn_create_guc_packet_sn(source, destination, 3);

            // Set up location table
            struct pico_gn_location_table_entry *entry = pico_gn_loct_add(&source_addr);
            entry->timestamp = 1104922580ul;
            entry->sequence_number = 2;
            
            fail_if(pico_gn_detect_duplicate_sntst_packet(f) != duplicate, "Error: a packet received with a timestamp older than the one in the LocTE and a sequence number that is equal.");
        }
    }
}
END_TEST

START_TEST(tc_pico_gn_alloc)
{
    { // GeoUnicast
        next_alloc_header_type = &guc_header_type;
        
        { // GeoUnicast with payload
            uint16_t payload_length = 0;
            uint16_t gn_size = PICO_SIZE_GNHDR + guc_header_type.size;
            struct pico_frame *frame = pico_gn_alloc(&pico_proto_geonetworking, payload_length);
            
            fail_if(frame->datalink_hdr != frame->buffer, "Error: the datalink header does not start at the start of the buffer.");
            fail_if(frame->net_hdr != (frame->buffer + PICO_SIZE_ETHHDR), "Error: the network layer header does not start at the correct position.");
            fail_if(frame->net_len != (gn_size), "Error: the network layer length is incorrect.");
            fail_if(frame->transport_hdr != (frame->net_hdr + gn_size), "Error: the transport layer header does not start at the start position.");
            fail_if(frame->transport_len != payload_length, "Error: the transport layer length is incorrect.");
            fail_if(frame->len != (payload_length + gn_size), "Error: length is incorrect.");
        }
        { // GeoUnicast without payload
            uint16_t payload_length = 50;
            uint16_t gn_size = PICO_SIZE_GNHDR + guc_header_type.size;
            struct pico_frame *frame = pico_gn_alloc(&pico_proto_geonetworking, payload_length);
            
            fail_if(frame->datalink_hdr != frame->buffer, "Error: the datalink header does not start at the start of the buffer.");
            fail_if(frame->net_hdr != (frame->buffer + PICO_SIZE_ETHHDR), "Error: the network layer header does not start at the correct position.");
            fail_if(frame->net_len != (gn_size), "Error: the network layer length is incorrect.");
            fail_if(frame->transport_hdr != (frame->net_hdr + gn_size), "Error: the transport layer header does not start at the start position.");
            fail_if(frame->transport_len != payload_length, "Error: the transport layer length is incorrect.");
            fail_if(frame->len != (payload_length + gn_size), "Error: length is incorrect.");
        }
    }
}
END_TEST

int pico_gn_guc_push_mock(struct pico_frame *frame)
{
    IGNORE_PARAMETER(frame);
    return 0;
}

START_TEST(tc_pico_gn_frame_sock_push)
{
   { // Test for header_type_guc, result should be 0 with the basic and common header as specified in the protocol.
        struct pico_gn_header_info guc_mock = guc_header_type;
        struct pico_gn_header *header = NULL;
        struct pico_frame *frame = NULL;
        struct pico_gn_data_request request = {
            .maximum_hop_limit = 156,
            .type = guc_mock,
            .lifetime = 123,
            .upper_proto = BTP_A,
        };
        
        guc_mock.push = pico_gn_guc_push_mock;
        next_alloc_header_type = &guc_mock;
        frame = pico_gn_alloc(&pico_proto_geonetworking, 0);
        frame->info = (void*)&request;
        
        pico_gn_frame_sock_push(&pico_proto_geonetworking, frame);
        
        header = (struct pico_gn_header*)frame->net_hdr;
        
        // Basic Header assertions
        fail_if(((header->basic_header.vnh & 0xF0) >> 4) != 0x00, "Error: the  protocol version of the Basic Header should be ETSI EN 302 636-4-1 (v1.2.1) (0).");
        fail_if((header->basic_header.vnh & 0x0F) != 1, "Error: the next header of the Basic Header should be the Common Header (1).");
        fail_if(header->basic_header.reserved != 0, "Error: reserved of the Basic Header should be set to 0.");
        fail_if(header->basic_header.remaining_hop_limit != request.maximum_hop_limit, "Error: remaining hop limit of the Basic Header should be set to the maximum hop limit given by the request.");
        fail_if(header->basic_header.lifetime != request.lifetime, "Error: lifetime of the Basic Header should be set to the lifetime given by the request.");
        
        // Common Header assertions
        fail_if(((header->common_header.next_header & 0xF0) >> 4) != (uint8_t)request.upper_proto, "Error: next header of the Common Header should be set to the upper protocol given by the request.");
        fail_if((header->common_header.next_header & 0x0F) != 0, "Error: the first reserved field of the Common Header should be set to 0.");
        fail_if(((header->common_header.header & 0xF0) >> 4) != 2, "Error: the header type of the Common Header should be set to GeoUnicast (2).");
        fail_if((header->common_header.next_header & 0x0F) != 0, "Error: the header sub-type of the Common Header should always be 0 for GeoUnicast.");
        fail_if(header->common_header.traffic_class.value != 0, "Error: the traffic class of the Common Header should be set to the default traffic class (0).");
        fail_if(header->common_header.payload_length != short_be(frame->payload_len), "Error: the payload length of the Common Header should be set to the payload length in big endian.");
        fail_if(header->common_header.maximum_hop_limit != request.maximum_hop_limit, "Error: the maximum hop limit of the Common Header should be set to the maximum hop limit given by the request.");
        fail_if(header->common_header.reserved_2 != 0, "Error: the second reserved field of the Common Header should be set to 0");
        
        PICO_FREE(frame);
    }
}
END_TEST

START_TEST(tc_pico_gn_abs)
{
    fail_if(pico_gn_abs(-1) != 1, "Error: the absolute value of -1 should be 1.");
    fail_if(pico_gn_abs(1) != 1, "Error: the absolute value of 1 should be 1.");
}
END_TEST

START_TEST(tc_pico_gn_sqrt)
{
    double result = 0;
    uint8_t is_valid = 1;
    { // Testing square root of 4, should result in 2.
        result = pico_gn_sqrt(4.0);
        is_valid = (result < 2.01 && result > 1.99);
        
        fail_if(!is_valid, "Error: square root of 2 should be 2.");
    }
    { // Testing the square root of 2.6, should result in 1.61245155
        result = pico_gn_sqrt(2.6);
        is_valid = (result < 1.6125 && result > 1.6120);
        
        fail_if(!is_valid, "Error: square root of 2.6 should be 1.61245");
    }
    { // Testing the square root of -5, should result in 0.
        result = pico_gn_sqrt(-5.0);
        is_valid = (result == 0);
        
        fail_if(!is_valid, "Error: square root of a negative number should be 0.");
    }
}
END_TEST

START_TEST(tc_pico_gn_calculate_distance)
{
    // The expected values are calculated using a existing Haversine implementation.
    { // Scenario one, the expected value is 0.01683 km.
        const int32_t expected = 16;
        const int32_t delta = 1;
                
        int32_t lat_a = 5144510, long_a = 544915;
        int32_t lat_b = 5144512, long_b = 544939;
        
        int32_t result = pico_gn_calculate_distance(lat_a, long_a, lat_b, long_b);
        
        fail_if(IN_DELTA(result, expected, delta), "Error: resulting value (%d) is not the same as the expected value (%d).", result, expected);
    }
    { // Scenario two, the expected value is 0.1230 km.
        const int32_t expected = 123;
        const int32_t delta = 1;
        
        int32_t lat_a = 5144353, long_a = 544997;
        int32_t lat_b = 5144245, long_b = 545037;
                
        int32_t result = pico_gn_calculate_distance(lat_a, long_a, lat_b, long_b);
        
        fail_if(IN_DELTA(result, expected, delta), "Error: resulting value (%d) is not the same as the expected value (%d).", result, expected);
    }
    { // Scenario three, the expected value is 0.6821 km.
        const int32_t expected = 682;
        const int32_t delta = 1;
        
        int32_t lat_a = 5142666, long_a = 540979;
        int32_t lat_b = 5143116, long_b = 541649;
                
        int32_t result = pico_gn_calculate_distance(lat_a, long_a, lat_b, long_b);
        
        fail_if(IN_DELTA(result, expected, delta), "Error: resulting value (%d) is not the same as the expected value (%d).", result, expected);
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
    
    TCase *TCase_pico_gn_link_add = tcase_create("Unit test for tc_pico_gn_link_add");
    tcase_add_test(TCase_pico_gn_link_add, tc_pico_gn_link_add);
    suite_add_tcase(s, TCase_pico_gn_link_add);
    
    TCase *TCase_pico_gn_link_compare = tcase_create("Unit test for pico_gn_link_compare");
    tcase_add_test(TCase_pico_gn_link_compare, tc_pico_gn_link_compare);
    suite_add_tcase(s, TCase_pico_gn_link_compare);
    
    TCase *TCase_pico_gn_address = tcase_create("Unit test for GeoNetworking address");
    tcase_add_test(TCase_pico_gn_address, tc_pico_gn_address);
    suite_add_tcase(s, TCase_pico_gn_address);
   
    TCase *TCase_pico_gn_get_time = tcase_create("Unit test for GeoNetworking getting the system time.");
    tcase_add_test(TCase_pico_gn_get_time, tc_pico_gn_get_time);
    suite_add_tcase(s, TCase_pico_gn_get_time);
            
    TCase *TCase_pico_gn_get_position = tcase_create("Unit test for GeoNetworking getting the Local Position.");
    tcase_add_test(TCase_pico_gn_get_position, tc_pico_gn_get_position);
    suite_add_tcase(s, TCase_pico_gn_get_position);
        
    TCase *TCase_pico_gn_fetch_frame_source_address = tcase_create("Unit test for pico_gn_fetch_frame_source_address.");
    tcase_add_test(TCase_pico_gn_fetch_frame_source_address, tc_pico_gn_fetch_frame_source_address);
    suite_add_tcase(s, TCase_pico_gn_fetch_frame_source_address);
    
    TCase *TCase_pico_gn_fetch_frame_sequence_number = tcase_create("Unit test for pico_gn_fetch_frame_sequence_number.");
    tcase_add_test(TCase_pico_gn_fetch_frame_sequence_number, tc_pico_gn_fetch_frame_sequence_number);
    suite_add_tcase(s, TCase_pico_gn_fetch_frame_sequence_number);
    
    TCase *TCase_pico_gn_fetch_frame_timestamp = tcase_create("Unit test for pico_gn_fetch_frame_timestamp.");
    tcase_add_test(TCase_pico_gn_fetch_frame_timestamp, tc_pico_gn_fetch_frame_timestamp);
    suite_add_tcase(s, TCase_pico_gn_fetch_frame_timestamp);
    
    TCase *TCase_pico_gn_fetch_loct_timestamp = tcase_create("Unit test for pico_gn_fetch_loct_timestamp.");
    tcase_add_test(TCase_pico_gn_fetch_loct_timestamp, tc_pico_gn_fetch_loct_timestamp);
    suite_add_tcase(s, TCase_pico_gn_fetch_loct_timestamp);
    
    TCase *TCase_pico_gn_fetch_loct_sequence_number = tcase_create("Unit test for pico_gn_fetch_loct_sequence_number.");
    tcase_add_test(TCase_pico_gn_fetch_loct_sequence_number, tc_pico_gn_fetch_loct_sequence_number);
    suite_add_tcase(s, TCase_pico_gn_fetch_loct_sequence_number);
    
    TCase *TCase_pico_gn_get_sequence_number = tcase_create("Unit test for pico_gn_get_sequence_number.");
    tcase_add_test(TCase_pico_gn_get_sequence_number, tc_pico_gn_get_sequence_number);
    suite_add_tcase(s, TCase_pico_gn_get_sequence_number);
    
    TCase *TCase_pico_gn_find_extended_header_length = tcase_create("Unit test for pico_gn_find_extended_header_length.");
    tcase_add_test(TCase_pico_gn_find_extended_header_length, tc_pico_gn_find_extended_header_length);
    suite_add_tcase(s, TCase_pico_gn_find_extended_header_length);
    
    TCase *TCase_pico_gn_get_header_info = tcase_create("Unit test for pico_gn_get_header_info.");
    tcase_add_test(TCase_pico_gn_get_header_info, tc_pico_gn_get_header_info);
    suite_add_tcase(s, TCase_pico_gn_get_header_info);
    
    TCase *TCase_pico_gn_detect_duplicate_sntst_packet = tcase_create("Unit test for pico_gn_detect_duplicate_sntst_packet.");
    tcase_add_test(TCase_pico_gn_detect_duplicate_sntst_packet, tc_pico_gn_detect_duplicate_sntst_packet);
    suite_add_tcase(s, TCase_pico_gn_detect_duplicate_sntst_packet);
    
    TCase *TCase_pico_gn_frame_sock_push = tcase_create("Unit test for pico_gn_frame_sock_push.");
    tcase_add_test(TCase_pico_gn_frame_sock_push, tc_pico_gn_frame_sock_push);
    suite_add_tcase(s, TCase_pico_gn_frame_sock_push);
    
    TCase *TCase_pico_gn_calculate_distance = tcase_create("Unit test for pico_gn_calculate_distance.");
    tcase_add_test(TCase_pico_gn_calculate_distance, tc_pico_gn_calculate_distance);
    suite_add_tcase(s, TCase_pico_gn_calculate_distance);
    
    TCase *TCase_pico_gn_alloc = tcase_create("Unit test for pico_gn_alloc.");
    tcase_add_test(TCase_pico_gn_alloc, tc_pico_gn_alloc);
    suite_add_tcase(s, TCase_pico_gn_alloc);
        
    TCase *TCase_pico_gn_sqrt = tcase_create("Unit test for pico_gn_sqrt.");
    tcase_add_test(TCase_pico_gn_sqrt, tc_pico_gn_sqrt);
    suite_add_tcase(s, TCase_pico_gn_sqrt);
    
    TCase *TCase_pico_gn_abs = tcase_create("Unit test for pico_gn_abs.");
    tcase_add_test(TCase_pico_gn_abs, tc_pico_gn_abs);
    suite_add_tcase(s, TCase_pico_gn_abs);
    
    return s;
}

#define CK_FORK "no"

int main(void)
{
    int fails;
    Suite *s = pico_suite();
    SRunner *sr = srunner_create(s);
    srunner_set_fork_status(sr, CK_NOFORK);
    srunner_run_all(sr, CK_NORMAL);
    fails = srunner_ntests_failed(sr);
    srunner_free(sr);    
    return fails;
}
