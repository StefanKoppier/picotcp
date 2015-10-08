/* 
 * File:   pico_geonetworking.h
 * Author: Stefan Koppier
 *
 * Created on 8 oktober 2015, 14:16
 */

#ifndef INCLUDE_PICO_GEONETWORKING
#define	INCLUDE_PICO_GEONETWORKING

#include "pico_protocol.h"

extern struct pico_protocol pico_proto_geonetworking;

/// Interface implementation which allows allocation of a GeoNetworking frame.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param size The size of the payload to be allocated.
///  \returns An GeoNetworking frame with the header sizes set correctly.
static struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size);

/// Interface implementation which allows the processing of incoming frames, from the Data Link Layer.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The which needs to be processed.
///  \returns 0 on success, -1 on failure.
static int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f);

/// Interface implementation which allows the processing of outgoing frames, from the Transport layer.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The which needs to be processed.
///  \returns 0 on success, -1 on failure.
static int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f);

/// Interface implementation which allows ???
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The which needs to be processed.
///  \returns 0 on success, -1 on failure.
static int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f);

#endif	/* INCLUDE_PICO_GEONETWORKING */