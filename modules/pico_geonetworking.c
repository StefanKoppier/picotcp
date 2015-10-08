#include "pico_geonetworking.h"

static struct pico_queue in = {0};
static struct pico_queue out = {0};

// Interface protocol definition 
struct pico_protocol pico_proto_geonetworking = {
    .name = "geonetworking",
    .proto_number = PICO_PROTO_GEONETWORKING,
    .layer = PICO_LAYER_NETWORK,
    .alloc = pico_gn_alloc,
    .process_in = pico_gn_process_in,
    .process_out = pico_gn_process_out,
    .push = pico_gn_frame_sock_push,
    .q_in = &in,
    .q_out = &out,
};

static struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size)
{
    // TODO: Implement function
    return NULL;
}

static int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

static int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}

static int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f)
{
    // TODO: Implement function
    return -1;
}