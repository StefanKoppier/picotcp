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

#endif /* INCLUDE_PICO_GEONETWORKING_GUC */

