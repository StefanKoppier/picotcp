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

#define PICO_GN_STATION_TYPE_UNKNOWN         0
#define PICO_GN_STATION_TYPE_PEDESTRIAN      1
#define PICO_GN_STATION_TYPE_CYCLIST         2
#define PICO_GN_STATION_TYPE_MOPED           3
#define PICO_GN_STATION_TYPE_MOTORCYCLE      4
#define PICO_GN_STATION_TYPE_PASSENGER_CAR   5
#define PICO_GN_STATION_TYPE_BUS             6
#define PICO_GN_STATION_TYPE_LIGHT_TRUCK     7
#define PICO_GN_STATION_TYPE_HEAVY_TRUCK     8
#define PICO_GN_STATION_TYPE_TRAILER         9
#define PICO_GN_STATION_TYPE_SPECIAL_VEHICLE 10
#define PICO_GN_STATION_TYPE_TRAM            11
#define PICO_GN_STATION_TYPE_ROADSIDE_UNIT   15

#define PICO_GN_LOCTE_STATION_TYPE_VEHICLE  0
#define PICO_GN_LOCTE_STATION_TYPE_ROADSIDE 1

#define PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY   0
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A 1
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B 2
#define PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6  3


extern struct pico_protocol pico_proto_geonetworking;
extern struct pico_tree     pico_gn_loct;

/// The method used to configure the GeoAdhoc router's GeoNetworking address.
enum pico_gn_address_conf_method
{
    AUTO = 0, ///< Auto-address configuration. Generate a random address.
    MANAGED = 1, ///< Managed address configuration. Request an address from the ITS Networking & Transport Layer Management entity. \bug This feature is not supported.
    ANONYMOUS = 2 ///< Anonymous address configuration. Request an address from the security entity. \bug This feature is not supported.
};


// The next GET and SET defines are probably broken.
#define PICO_GET_GNADDR_MANUAL(x) (x & 0x01)
#define PICO_GET_GNADDR_STATION_TYPE(x) ((x >> 1) & 0x1F)
#define PICO_GET_GNADDR_COUNTRY_CODE(x) ((x >> 6) & 0x3FF)
#define PICO_GET_GNADDR_MID(x) (x >> 16)

#define PICO_SET_GNADDR_MANUAL(x, v) x = ((x & (~(uint64_t)0x1)) | (v & (uint64_t)0x1))
#define PICO_SET_GNADDR_STATION_TYPE(x, v) x = ((x & (~(uint64_t)0x3E)) | (v & (uint64_t)0x3E))
#define PICO_SET_GNADDR_COUNTRY_CODE(x, v) x = ((x & (~(uint64_t)0xFFC0)) | (v & (uint64_t)0xFFC0))
#define PICO_SET_GNADDR_MID(x, v) x = ((x & (~(uint64_t)0xFFFFFFFFFFFF0000)) | (v & (uint64_t)0xFFFFFFFFFFFF0000))

#define PICO_SIZE_GNADDRESS ((uint32_t)sizeof(struct pico_gn_address))
/// The GeoNetworking address that uniquely identifies a GeoNetworking entity.
PACKED_STRUCT_DEF pico_gn_address
{
    uint64_t value; ///< Contains all values for the GeoNetworking address. The first bit determines whether the address was manually set using \enum pico_gn_address_conf _method.AUTO, this means 0 when the address if manually configures, 1 if otherwise. The next five bits determine the type of the ITS-station. The next ten bits determine the ITS-station country code as described in 'ITU Operational Bulletin No. 741 - 1.VI.2001'. The last 48 bit represent the Logic Link Address.
};

#define PICO_SIZE_GNLOCTE ((uint32_t)sizeof(struct pico_gn_location_table_entry))
/// The Location Table entry that are contained in the Location Table
struct pico_gn_location_table_entry
{
    struct pico_gn_address *address; ///< The GeoNetworking address of the ITS-station
    uint64_t                ll_address; ///< The 48 bit physical address of the ITS-station
    uint8_t                 station_type; ///< The type of the ITS-station. This value should be either PICO_GN_LOCTE_STATION_TYPE_VEHICLE or PICO_GN_LOCTE_STATION_TYPE_ROADSIDE.
    uint8_t                 proto_version; ///< The four bit protocol version executed by the ITS-station.
    struct pico_gn_lpv     *position_vector; ///< The Long Position Vector of the ITS-station. The GeoNetworking address might not be set.
    uint8_t                 location_service_pending; ///< Boolean indicating that a Location Service for this GeoNetworking address is in progress.
    uint8_t                 is_neighbour; ///< Boolean indicating that the GeoAdhoc router is a direct neighbour.
    uint16_t                sequence_number; ///< The last sequence number received from this GeoNetworking address that was identified as 'not duplicated'.
    uint32_t                timestamp; ///< The timestamp of the last packet reveiced from this Geonetworking address that was identifed as 'not duplicated'.
    uint16_t                packet_data_rate; ///< The Packet data rate as Exponential Moving Average.
};

#define PICO_SIZE_GNLINK ((uint32_t)sizeof(struct pico_gn_link))
/// The link between the \struct pico_device and the \struct pico_gn_address
struct pico_gn_link
{
    struct pico_device *dev; ///< The device which the address is coupled to.
    struct pico_gn_address address; ///< The address which the device is coupled to.
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

// THE NEXT TWO DEFINES ARE PROBABLY BROKEN
#define PICO_GET_GNLPV_ACCURACY(x) (x & 0x01))
#define PICO_GET_GNLPV_SPEED(x) (x >> 1)
#define PICO_SIZE_GNLPV ((uint32_t)sizeof(struct pico_gn_lpv))
/// The Long Position Vector containing all position-related information.
PACKED_STRUCT_DEF pico_gn_lpv
{
    struct pico_gn_spv short_pv; ///< The Short Position Vector containing the GeoNetworking address, timestamp, latitude and longitude.
    uint16_t           sac; ///< The frist bit indicates the accuracy of the reference position. The last 15 bit indicata the speed expressed in 0.01 metre per second.
    uint16_t           heading; ///< Heading expressed in 0.1 degree from North.
};

#define PICO_GET_GNBASICHDR_VERSION(x) ((x & 0xF0) >> 4)
#define PICO_GET_GNBASICHDR_NEXT_HEADER(x) (x & 0x0F)
#define PICO_SIZE_GNBASICHDR ((uint32_t)sizeof(struct pico_gn_basic_header))
/// The Basic Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_basic_header 
{
    uint8_t vnh; ///< The first four bits represent the version of the GeoNetworking protocol. The last four bits identifies the header immediately following the GeoNetworking Basic Header.
    uint8_t reserved; ///< Reserved, should be set to 0.
    uint8_t lifetime; ///< Indicates the maximum tolerable time a packet can be buffered until it reaches its destination.
    uint8_t remaining_hop_limit; ///< Decrembented by 1 by each GeoAdhoc router that forwards the packet. The packet shall not be forwarded if RHL is decremented to zero.
};

#define PICO_GET_GNCOMMONHDR_NEXT_HEADER(x) ((x & 0xF0) >> 4)
#define PICO_GET_GNCOMMONHDR_HEADER(x) ((x & 0xF0) >> 4)
#define PICO_GET_GNCOMMONHDR_SUBHEADER(x) (x & 0x0F)
#define PICO_SIZE_GNCOMMONHDR ((uint32_t)sizeof(struct pico_gn_common_header))
/// The Common Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_common_header
{
    uint8_t  next_header; ///< The first four bits identify the type of header immediately following the GeoNetworking headers. This value should be one of the following: PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B or PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6. The last four bits are reserved and must be set to 0.
    uint8_t  header; ///< The first four bits identify the type of the GeoNetworking extended header The last four bits identify the sub-type of the GeoNetworking extended header.
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

#define PICO_SIZE_LSREQHDR ((uint32_t)sizeof(struct pico_gn_lsreq_header))
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

/// Function for adding this \struct pico_device to the GeoAdhoc router.
/// This function will create the GeoNetworking address according to the \struct pico_gn_address_config_method given.
///  \param dev The device to add to the GeoAdhoc router.
///  \param method The configuration method used to create the GeoNetworking address.
///  \param station_type The type of ITS-station. This value should be one of the following: PICO_GN_STATION_TYPE_UNKNOWN, PICO_GN_STATION_TYPE_PEDESTRIAN, PICO_GN_STATION_TYPE_CYCLIST, PICO_GN_STATION_TYPE_MOPED, PICO_GN_STATION_TYPE_MOTORCYCLE, PICO_GN_STATION_TYPE_PASSENGER_CAR, PICO_GN_STATION_TYPE_BUS, PICO_GN_STATION_TYPE_LIGHT_TRUCK, PICO_GN_STATION_TYPE_HEAVY_TRUCK, PICO_GN_STATION_TYPE_TRAILER, PICO_GN_STATION_TYPE_SPECIAL_VEHICLE, PICO_GN_STATION_TYPE_TRAM or PICO_GN_STATION_TYPE_ROADSIDE_UNIT.
///  \param country_code The country code of the ITS-station as described in 'ITU Operational Bulletin No. 741 - 1.VI.2001'.
///  \returns 0 on success, else -1.
int pico_gn_link_add(struct pico_device *dev, enum pico_gn_address_conf_method method, uint8_t station_type, uint16_t country_code);

/// Function for creating a GeoNetworking address with a random MID field.
///  \param result The result of this function as a \struct pico_gn_address.
///  \param station_type The type of ITS-station. This value should be one of the following: PICO_GN_STATION_TYPE_UNKNOWN, PICO_GN_STATION_TYPE_PEDESTRIAN, PICO_GN_STATION_TYPE_CYCLIST, PICO_GN_STATION_TYPE_MOPED, PICO_GN_STATION_TYPE_MOTORCYCLE, PICO_GN_STATION_TYPE_PASSENGER_CAR, PICO_GN_STATION_TYPE_BUS, PICO_GN_STATION_TYPE_LIGHT_TRUCK, PICO_GN_STATION_TYPE_HEAVY_TRUCK, PICO_GN_STATION_TYPE_TRAILER, PICO_GN_STATION_TYPE_SPECIAL_VEHICLE, PICO_GN_STATION_TYPE_TRAM or PICO_GN_STATION_TYPE_ROADSIDE_UNIT.
///  \param country_code The country code of the ITS-station as described in 'ITU Operational Bulletin No. 741 - 1.VI.2001'.
///  \returns 0 on success, -1 on failure.
int pico_gn_create_address_auto(struct pico_gn_address *result, uint8_t station_type, uint16_t country_code);

/// Function for fetching a GeoNetworking address from the ITS Networking & Transport Layer Management entity.
///  \bug Not implemented.
///  \param result The result of this function as a \struct pico_gn_address. This value will be unchanged by this function.
///  \returns -1.
int pico_gn_create_address_managed(struct pico_gn_address *result);

/// Function for fetching a GeoNetworking address from the security entity.
///  \bug Not implemented.
///  \param result The result of this function as a \struct pico_gn_address. This value will be unchanged by this function.
///  \returns -1.
int pico_gn_create_address_anonymous(struct pico_gn_address *result);

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
/// Receiving of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_receive(struct pico_frame *f);
/// Forwarding of an incoming GeoUnicast packet.
///  \param f The frame which contains the GeoUnicast header.
///  \returns 0 on success, -1 on failure.
int pico_gn_process_guc_forward(struct pico_frame *f);
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
///  \param station_type Flag determining if the station is PICO_GN_LOCTE_STATION_TYPE_VEHICLE or PICO_GN_LOCTE_STATION_TYPE_ROADSIDE.
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
int pico_gn_detect_duplicate_sntst_packet(struct pico_frame *f);

/// Method for determining if the received BEACON or SHB is a duplicate.
/// This is determined using a method based on the timestamp.
///  \param f The frame which needs to be processed.
///  \returns 0 when not duplicate, 1 when duplicate, -1 on failure.
int pico_gn_detect_duplicate_tst_packet(struct pico_frame *f);

/// Method for fetching the source GeoNetworking address out of the extended header from the \struct pico_frame.
///  \param f The \struct pico_frame containing the GeoNetworking headers used to find the \struct pico_gn_address.
///  \returns The found source GeoNetworking address on success, NULL on failure.
struct pico_gn_address *pico_gn_fetch_frame_source_address(struct pico_frame *f);

/// Method for fetching the sequence number out of the extended header from the \struct pico_frame.
///  \param f The \struct pico_frame containing the GeoNetworking headers used to find the sequence number.
///  \returns The (16-bit) sequence number on success, -1 on failure.
int32_t pico_gn_fetch_frame_sequence_number(struct pico_frame *f);

/// Method for fetching the timestamp out of the extended header from the \struct pico_frame.
///  \param f The \struct pico_frame containing the GeoNetworking headers used to find the timestamp.
///  \returns The (32-bit) timestamp on success, -1 on failure.
int64_t pico_gn_fetch_frame_timestamp(struct pico_frame *f);

/// Method for fetching the last received sequence number out of the LocT from a GeoNetworking address.
///  \param addr The \struct pico_gn_address used to find the sequence number.
///  \returns The (16-bit) sequence number on success, -1 when not found.
int32_t pico_gn_fetch_loct_sequence_number(struct pico_gn_address *addr);

/// Method for fetching the last received timestamp out of the LocT from a GeoNetworking address.
///  \param addr The \struct pico_gn_address used to find the timestamp.
///  \returns The (32-bit) sequence number on success, -1 when not found.
int64_t pico_gn_fetch_loct_timestamp(struct pico_gn_address *addr);

/// Method for achieving uniqueness of the GeoNetworking address of the local system.
/// This is done when a packet is received. The GeoAdhoc router checks is the received address is equals to the local address.
/// If so, the local address shall be updated.
///  \param f The received frame to check against.
void pico_gn_detect_duplicate_address(struct pico_frame *f);

/// Method for comparing two \struct pico_gn_link structs.
/// This function is used by the \struct pico_tree to insert, find and delete a LocTE inside the \struct pico_tree.
///  \param a The reference to the first \struct pico_gn_link.
///  \param b The reference to the second \struct pico_gn_link.
///  \returns -1 when a < than b, 1 when a > b, else 0
int pico_gn_link_compare(void *a, void *b);

/// Method for comparing two Location Table entries.
/// This function is used by the \struct pico_tree to insert, find and delete a LocTE inside the \struct pico_tree.
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