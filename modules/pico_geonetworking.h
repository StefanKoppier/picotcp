/* 
 * File:   pico_geonetworking.h
 * Author: Stefan Koppier
 *
 * Created on 8 oktober 2015, 14:16
 */

#ifndef INCLUDE_PICO_GEONETWORKING
#define INCLUDE_PICO_GEONETWORKING

#include "pico_protocol.h"
#include "pico_config.h"


#define PICO_GN_PROTOCOL_VERSION 0 // EN 302 636-4-1 v1.2.1

#define PICO_GN_STATION_TYPE_ROADSIDE 0
#define PICO_GN_STATION_TYPE_VEHICLE  1

#define PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY   0
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A 1
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B 2
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6  3


extern struct pico_protocol   pico_proto_geonetworking;
extern struct pico_tree       pico_gn_location_table;
extern struct pico_gn_address pico_gn_local_address;

#define PICO_SIZE_GNADDRESS ((uint32_t)sizeof(struct pico_gn_address))
/// The GeoNetworking address that uniquely identifies a GeoNetworking entity.
PACKED_STRUCT_DEF pico_gn_address
{
    uint8_t  manual: 1; ///< 0 when the address if manually configures, 1 if otherwise.
    uint8_t  station_type: 5; ///< The type of the ITS-station.
    uint16_t country_code: 10; ///< The ITS-station Country Code.
    uint64_t ll_address: 48; ///< The Logic Link address of the GeoAdhoc router
};

#define PICO_SIZE_GNLOCTE ((uint32_t)sizeof(struct pico_gn_location_table_entry))
/// The Location Table entry that are contained in the Location Table
struct pico_gn_location_table_entry
{
    struct pico_gn_address *address; ///< The GeoNetworking address of the ITS-station
    uint64_t                ll_address: 48; ///< The physical address of the ITS-station
    uint8_t                 station_type: 1; ///< The type of the ITS-station. This value should be either PICO_GN_STATION_TYPE_ROADSIDE or PICO_GN_STATION_TYPE_VEHICLE
    uint8_t                 proto_version: 4; ///< The protocol version executed by the ITS-station.
    struct pico_gn_lpv     *position_vector; ///< The Long Position Vector of the ITS-station. The GeoNetworking address might not be set.
    uint8_t                 location_service_pending: 1; ///< Flag indicating that a Location Service for this GeoNetworking address is in progress.
    uint8_t                 is_neighbour: 1; ///< Flag indicating that the GeoAdhoc router is a direct neighbour.
    uint16_t                sequence_number; ///< The last sequence number received from this GeoNetworking address that was identified as 'not duplicated'.
    uint32_t                timestamp; ///< The timestamp of the last packet reveiced from this Geonetworking address that was identifed as 'not duplicated'.
    uint16_t                packet_data_rate; ///< The Packet data rate as Exponential Moving Average.
};

#define PICO_SIZE_GNSPV ((uint32_t)sizeof(struct pico_gn_spv))
/// The Short Position Vector containing the minimum position-related information.
PACKED_STRUCT_DEF pico_gn_spv
{
    struct pico_gn_address address; ///< The GeoNetworking address of which the Position Vector originated.
    uint32_t               timestamp; ///< The time in milliseconds at which the latitude and longitude were acuired.
    uint32_t               latitude; ///< The latitude of the GeoAdhoc router reference position expressed in 1/10 micro degree.
    uint32_t               longitude; ///< The longitude of the GeoAdhoc router reference position expresssed in 1/10 micro degree.
};

#define PICO_SIZE_GNLPV ((uint32_t)sizeof(struct pico_gn_lpv))
/// The Long Position Vector containing all position-related information.
PACKED_STRUCT_DEF pico_gn_lpv
{
    struct pico_gn_spv short_pv; ///< The Short Position Vector containing the GeoNetworking address, timestamp, latitude and longitude.
    uint8_t            position_accuracy_indicator: 1; ///< Flag indicating the accuracy of the reference position. 1 when 
    int16_t            speed: 15; ///< Speed expressed in 0.01 metre per second.
    uint16_t           heading; ///< Heading expressed in 0.1 degree from North.
};

#define PICO_SIZE_GNBASICHDR ((uint32_t)sizeof(struct pico_gn_basic_header))
/// The Basic Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_basic_header 
{
    uint8_t version: 4; ///< Identifies the version of the GeoNetworking protocol.
    uint8_t next_header: 4; ///< Identifies the header immediately following the GeoNetworking Basic Header.
    uint8_t reserved; ///< Reserved, should be set to 0.
    uint8_t lifetime; ///< Indicates the maximum tolerable time a packet can be buffered until it reaches its destination.
    uint8_t remaining_hop_limit; ///< Decrembented by 1 by each GeoAdhoc router that forwards the packet. The packet shall not be forwarded if RHL is decremented to zero.
};

#define PICO_SIZE_GNCOMMONHDR ((uint32_t)sizeof(struct pico_gn_basic_header))
/// The Common Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_common_header
{
    uint8_t  next_header: 4; ///< Identifies the type of header immediately following the GeoNetworking headers. This value should be one of the following: PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B or PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6
    uint8_t  reserved_1: 4; ///< Reserved should be set to 0.
    uint8_t  header: 4; ///< Identifies the type of the GeoNetworking extended header.
    uint8_t  subheader: 4; ///< Identifies the sub-type of the GeoNetworking extended header.
    uint8_t  traffic_class; ///< Traffic class that represents Facility-layer requirements on packet transport.
    uint8_t  flags; ///< Bit 0: Indicates whether the ITS-S is mobile or stationary. Bit 1 to 7: Reserved, should be set to 0.
    uint16_t payload_length; ///< Length of the GeoNetworking payload, i.e. the rest of the packet following the whole GeoNetworking header in octets.
    uint8_t  maximum_hop_limit; ///< Maximum hop limit, this value is NOT decremented by a GeoAdhoc router that forwards the packet.
    uint8_t  reserved_2; ///< Reserved, should be set to 0.
};

#define PICO_SIZE_GNHDR ((uint32_t)sizeof(struct pico_gn_header))
/// The combined Basic Header and Common Header which are available in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_header
{
    struct pico_gn_basic_header  basic_header; ///< The Basic Header of the GeoNetworking packet.
    struct pico_gn_common_header common_header; ///< The Common Header of the GeoNetworking packet.
};

#define PICO_SIZE_GUCHDR ((uint32_t)sizeof(struct pico_gn_guc_header))
/// The GeoUnicast header for GeoUnicast packets.
PACKED_STRUCT_DEF pico_gn_guc_header
{
    uint16_t           sequence_number; ///< Indicates the index of the sent GUC packet and used to detect duplicate packets.
    uint16_t           reserved; ///< Reserved, should be set to 0.
    struct pico_gn_lpv source; ///< Source Long Position Vector containing the reference position of the source.
    struct pico_gn_spv destination; ///< Destination Short Position Vector containing the position of the destination.
};

#define PICO_SIZE_TSCHDR ((uint32_t)sizeof(struct pico_gn_tsc_header))
/// The Topologically-scoped broadcast header Topologically-scoped broadcast packets.
PACKED_STRUCT_DEF pico_gn_tsc_header
{
    
};

#define PICO_SIZE_SHBHDR ((uint32_t)sizeof(struct pico_gn_shb_header))
/// The Single-hop broadcast header for Single-hop broadcast packets.
PACKED_STRUCT_DEF pico_gn_shb_header
{
    
};

#define PICO_SIZE_GBCHDR ((uint32_t)sizeof(struct pico_gn_gbc_header))
/// The GeoBroadcast header for GeoBroadcast packets.
PACKED_STRUCT_DEF pico_gn_gbc_header
{
    
};

#define PICO_SIZE_BEACONHDR ((uint32_t)sizeof(struct pico_gn_beacon_header))
/// The GeoAnycast header for GeoAnycast packets.
PACKED_STRUCT_DEF pico_gn_beacon_header
{
    
};

#define PICO_SIZE_LSRREQHDR ((uint32_t)sizeof(struct pico_gn_lsreq_header))
/// The Location Service request header for Location Service request packets.
PACKED_STRUCT_DEF pico_gn_lsreq_header
{
    
};

#define PICO_SIZE_LSRESHDR ((uint32_t)sizeof(struct pico_gn_lsres_header))
/// The Location Service response header for Location Service response packets.
PACKED_STRUCT_DEF pico_gn_lsres_header
{
    
};

#define PICO_SIZE_GNDATA_INDICATION ((uint32_t)sizeof(struct pico_gn_data_indication))
/// The GN-DATA.indication primitive which contains information about the received GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_data_indication
{
    
};


/// Interface implementation which allows allocation of a GeoNetworking frame.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param size The size of the payload to be allocated.
///  \returns An GeoNetworking frame with the header sizes set correctly.
struct pico_frame *pico_gn_alloc(struct pico_protocol *self, uint16_t size);

/// Interface implementation which allows the processing of incoming frames, from the Data Link Layer.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The which needs to be processed.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_in(struct pico_protocol *self, struct pico_frame *f);
/// Processing of an incoming Beacon packet.
///  \param f The frame which contains the Beacon header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_beacon_in(struct pico_frame *f);
/// Processing of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_in(struct pico_frame *f);
/// Processing of an incoming GeoAnycast packet.
///  \param f The frame which contains the GeoAnycast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_gac_in(struct pico_frame *f);
/// Processing of an incoming GeoBroadcast packet.
///  \param f The frame which contains the GeoBroadcast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_gbc_in(struct pico_frame *f);
/// Processing of an incoming Topologically-scoped broadcast multi-hop packet.
///  \param f The frame which contains the Topologically-scoped broadcast multi-hop header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_mh_in(struct pico_frame *f);
/// Processing of an incoming Topologically-scoped broadcast single-hop packet.
///  \param f The frame which contains the Topologically-scoped broadcast single-hop header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_sh_in(struct pico_frame *f);
/// Processing of an incoming Location Service packet.
///  \param f The frame which contains the Location Service header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_ls_in(struct pico_frame *f);

/// Interface implementation which allows the processing of outgoing frames, from the Transport layer.
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The frame which needs to be processed.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_out(struct pico_protocol *self, struct pico_frame *f);

/// Interface implementation which allows ???
///  \param self The protocol definition, this protocol will always be pico_proto_geonetworking.
///  \param f The frame which needs to be processed.
///  \returns 0 on success, -1 on failure.
int pico_gn_frame_sock_push(struct pico_protocol *self, struct pico_frame *f);

/// Method that tries to find a \struct pico_gn_location_table_entry inside the Location Table using a \struct pico_gn_address.
///  \param address The address that should be searched for.
///  \returns The sought LocTE, NULL if not found.
struct pico_gn_location_table_entry* pico_gn_loct_find(struct pico_gn_address *address);

/// Method for updating a \struct pico_gn_location_table_entry with a new \struct pico_gn_lpv.
///  \param address The address of which the \struct pico_gn_lpv should be updated.
///  \param vector The new \struct pico_gn_lpv which contains the new values.
///  \param is_neighbour Flag determining if this \struct pico_gn_lpv is a direct neighbour.
///  \param sequence_number The sequence number of the last received packet from this address which was not found to be a duplicate.
///  \param station_type Flag determining if the station is PICO_GN_STATION_TYPE_ROADSIDE or PICO_GN_STATION_TYPE_VEHICLE
///  \param timestamp The timestamp of the last received packet from this address which was not found to be a duplicate.
///  \returns 0 on success, -1 when the /struct pico_gn_address is not found
int pico_gn_loct_update(struct pico_gn_address *address, struct pico_gn_lpv *vector, uint8_t is_neighbour, uint16_t sequence_number, uint8_t station_type, uint32_t timestamp);

/// Method for adding a \struct pico_gn_location_table_entry to the Location Table
/// When the Table already contains an entry for the \struct pico_gn_address, the existing entry shall be returned.
///  \param address The address for which the \struct pico_gn_lpv should be added.
///  \returns A pointer to the inserted or existing \struct pico_gn_location_table_entry, NULL on failure.
struct pico_gn_location_table_entry *pico_gn_loct_add(struct pico_gn_address *address);

/// Determines the size of the extended header according to the header and subheader fields of the Common Header.
///  \param header The header which contains the header and subheader that will follow the Common Header.
///  \returns The extended header length in octets, 0 if the header type is of any type, -1 on failure.
int pico_gn_find_extended_header_length(struct pico_gn_header *header);

/// Method for determining if the received GUC, TSB, GAC, LS Request or LS Response packet is a duplicate.
/// This is determined using a method based on the sequence number and the timestamp.
///  \param f The frame which needs to be processed.
///  \returns 0 when not duplicate, 1 when duplicate, -1 on failure.
int pico_gn_detect_duplicate_SNTST_packet(struct pico_frame *f);

/// Method for determining if the received BEACON or SHB is a duplicate.
/// This is determined using a method based on the timestamp.
///  \param f The frame which needs to be processed.
///  \returns 0 when not duplicate, 1 when duplicate, -1 on failure.
int pico_gn_detect_duplicate_TST_packet(struct pico_frame *f);

/// Method for achieving uniqueness of the GeoNetworking address of the local system.
/// This is done when a packet is received. The GeoAdhoc router checks is the received address is equals to the local address.
/// If so, the local address shall be updated.
///  \param f The received frame to check against.
void pico_gn_detect_duplicate_address(struct pico_frame *f);

/// Method for comparing two Location Table entries.
/// This function is used by the pico_queue to insert, find and delete a LocTE inside the \struct pico_queue.
///  \param a The reference to the first \struct pico_gn_location_table_entry
///  \param b The reference to the second \struct pico_gn_location_table_entry
///  \returns -1 when a < than b, 1 when a > b, else 0
int pico_gn_locte_compare(void *a, void *b);

/// Checks if two GeoNetworking addresses are equal.
///  \param a The reference to the first \struct pico_gn_address.
///  \param b The reference to the second \struct pico_gn_address.
///  \returns 1 when equal, else 0
int pico_gn_address_equals(struct pico_gn_address *a, struct pico_gn_address *b);

#endif	/* INCLUDE_PICO_GEONETWORKING */