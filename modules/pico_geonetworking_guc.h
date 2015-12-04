/* 
 * File:   pico_geonetworking_guc.h
 * Author: stefan
 *
 * Created on November 2, 2015, 2:49 PM
 */

#ifndef INCLUDE_PICO_GEONETWORKING_GUC
#define INCLUDE_PICO_GEONETWORKING_GUC

#include "pico_geonetworking_common.h"
#include "pico_config.h"
#include "pico_stack.h"

#define PICO_GN_GUC_GREEDY_FORWARDING_ALGORITHM           1
#define PICO_GN_GUC_CONTENTION_BASED_FORWARDING_ALGORITHM 2 // Not supported in this implementation, only greedy is.
#define PICO_GN_GUC_FORWARDING_ALGORITHM                  PICO_GN_GUC_GREEDY_FORWARDING_ALGORITHM

extern const struct pico_gn_header_info guc_header_type; 

#define PICO_SIZE_GUCHDR ((uint32_t)sizeof(struct pico_gn_guc_header))
/// The GeoUnicast header for GeoUnicast packets.
PACKED_STRUCT_DEF pico_gn_guc_header
{
    uint16_t           sequence_number; ///< Indicates the index of the sent GUC packet and used to detect duplicate packets.
    uint16_t           reserved; ///< Reserved, should be set to 0.
    struct pico_gn_lpv source; ///< Source Long Position Vector containing the reference position of the source.
    struct pico_gn_spv destination; ///< Destination Short Position Vector containing the position of the destination.
};

/// Function to send a GeoUnicast packet.
/// This function should only be called directly for debugging/testing purposes, sending from a layer above GeoNetworking should be done using the push function.
///  \param destination The destination of this GeoUnicast packet.
///  \returns 0 on success, -1 on failure.
int pico_gn_guc_send(struct pico_gn_address *destination);

/// Enqueues an initialized packet with an initialized GeoUnicast header into the outgoing queue.
///  \param f The frame representing the complete packet.
///  \returns -1 on failure, 0 on success.
int pico_gn_guc_push(struct pico_frame *f);

/// Allocating of an GeoUnicast frame.
///  \param size The size of the payload.
///  \returns The allocated GUC frame, NULL on failure.
struct pico_frame *pico_gn_guc_alloc(uint16_t size);

/// Processing of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_in(struct pico_frame *f);

/// Receiving of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_receive(struct pico_frame *f);

/// Forwarding of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_forward(struct pico_frame *f);

/// Processing of an outgoing GeoUnicast packet.
///  \param f Te frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_out(struct pico_frame *f);

/// Function for determining the next hop.
///  \param dest The final destination of the packet.
///  \param traffic_class The traffic class of the packet.
///  \results Returns The MID field of the next hop, 0 if the packet is added to the forwarding buffer or a broadcast address if no next hop was found.
uint64_t pico_gn_guc_greedy_forwarding(const struct pico_gn_spv *dest, const struct pico_gn_traffic_class *traffic_class);

#endif /* INCLUDE_PICO_GEONETWORKING_GUC */

