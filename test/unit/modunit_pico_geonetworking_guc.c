#include "modules/pico_geonetworking_common.c"
#include "check.h"
#include "pico_geonetworking_guc.h"

Suite *pico_suite(void)
{
    Suite *s = suite_create("GeoNetworking module");
    
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
