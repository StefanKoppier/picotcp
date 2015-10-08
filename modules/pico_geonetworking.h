/* 
 * File:   pico_geonetworking.h
 * Author: Stefan Koppier
 *
 * Created on 8 oktober 2015, 14:16
 */

#ifndef INCLUDE_PICO_GEONETWORKING
#define	INCLUDE_PICO_GEONETWORKING

#include "pico_protocol.h"

extern struct pico_protocol *pico_proto_geonetworking;


static struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size);
static int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f);
static int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f);
static int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f);

#endif	/* INCLUDE_PICO_GEONETWORKING */