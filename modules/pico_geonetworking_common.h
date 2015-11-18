/* 
 * File:   pico_geonetworking_common.h
 * Author: Stefan Koppier
 *
 * Created on 8 oktober 2015, 14:16
 */

#ifndef INCLUDE_PICO_GEONETWORKING_COMMON
#define INCLUDE_PICO_GEONETWORKING_COMMON

#include "pico_protocol.h"
#include "pico_config.h"
#include "pico_tree.h"

#include "pico_geonetworking_management.h"

#define PICO_GN_PROTOCOL_VERSION 0 // EN 302 636-4-1 v1.2.1

#define PICO_GN_LOCTE_STATION_TYPE_VEHICLE  0
#define PICO_GN_LOCTE_STATION_TYPE_ROADSIDE 1

/// Enum containing the different types of ITS-stations.
enum pico_gn_station_type
{
    UNKNOWN         = 0,
    PEDESTRIAN      = 1,
    CYCLIST         = 2,
    MOPED           = 3,
    MOTORCYCLE      = 4,
    PASSENGER_CAR   = 5,
    BUS             = 6,
    LIGHT_TRUCK     = 7,
    HEAVY_TRUCK     = 8,
    TRAILER         = 9,
    SPECIAL_VEHICLE = 10,
    TRAM            = 11,
    ROADSIDE_UNIT   = 15,
};

/// Enum containing the possible next values in the \struct pico_gn_basic_header next header field.
enum pico_gn_basic_header_next_header
{
    BH_ANY  = 0, ///< The protocol treats this field as if the next header is a Common Header.
    COMMON  = 1,
    SECURED = 2, ///< \bug NOT SUPPORTED.
};

enum pico_gn_common_header_next_header
{
    CM_ANY = 0, ///< The protocol does not specify what the next header is. This packet will not be forwarded to another transport or networking layer.
    BTP_A  = 1,
    BTP_B  = 2,
    GN6ASL = 3,
};

extern struct pico_protocol pico_proto_geonetworking;
extern struct pico_tree     pico_gn_loct;
extern struct pico_tree     pico_gn_dev_link;

/// The method used to configure the GeoAdhoc router's GeoNetworking address.
enum pico_gn_address_conf_method
{
    AUTO = 0, ///< Auto-address configuration. Generate a random address.
    MANAGED = 1, ///< Managed address configuration. Request an address from the ITS Networking & Transport Layer Management entity. \bug This feature is not supported.
    ANONYMOUS = 2 ///< Anonymous address configuration. Request an address from the security entity. \bug This feature is not supported.
};

/// The logic link protocol to use beneath the GeoNetworking protocol.
enum pico_gn_communication_profile
{
    UNSPECIFIED = 0, ///< Unspecified protocol. This implemention of GeoNetworking assumes Ethernet.
    ITSG5 = 1 ///< ITS-G5. NOT SUPPORTED.
};

typedef int                (*process_in)  (struct pico_frame*);
typedef int                (*process_out) (struct pico_frame*);
typedef int                (*socket_push) (struct pico_frame*);
typedef struct pico_frame *(*alloc)       (uint16_t);

/// Struct containing information about a specfic extended header.
/// It's used in the extended header processing and finding specific fields in these extended headers.
struct pico_gn_header_info
{
    uint8_t     header: 4; ///< Field containing the extended header type. These values are determined by the GeoNetworking protocol.
    uint8_t     subheader: 4; ///< Field containing the extended header sub-type. These values are determined by the GeoNetworking protocol.
    uint32_t    size; ///< The size of the extended header in bytes.
    struct
    {
        int32_t source_address; ///< The position of the source address.
        int32_t sequence_number; ///< The position of the sequence number.
        int32_t timestamp; ///< The position of the timestamp.
    } offsets; ///< Complex field containing the offsets of specific fields in the extended header in bytes. This offset starts at the beginning of the extended header.
    process_in  in; ///< Function pointer to the extended header inward processing function.
    process_out out; ///< Function pointer to the extended header outward processing function.
    socket_push push; ///< Function pointer to the exteneded header socket push function.
    alloc       alloc; ///< Function pointer to the extended header alloc function.
};

extern const struct pico_gn_header_info header_info_invalid;

#define PICO_GET_GNADDR_MANUAL(x) ((uint8_t)((x >> 7) & 0x01))
#define PICO_GET_GNADDR_STATION_TYPE(x) ((uint8_t)((x >> 2) & 0x1F))
#define PICO_GET_GNADDR_COUNTRY_CODE(x) ((uint16_t)(short_be(((uint16_t*)&x)[0]) & 0x3FF))
#define PICO_GET_GNADDR_MID(x) ((uint64_t)(x >> 16))

#define PICO_SET_GNADDR_MANUAL(x, v) x = (((x & (~(uint64_t)0x80))) | ((v << 7) & 0x80))
#define PICO_SET_GNADDR_STATION_TYPE(x, v) x = (((x & (~(uint64_t)0x7C))) | ((v << 2) & 0x7C))
#define PICO_SET_GNADDR_COUNTRY_CODE(x, v) x = (((x & (~(uint64_t)0xFF03))) | (short_be(v) & 0xFF03))
#define PICO_SET_GNADDR_MID(x, v) x = (((x & (~(uint64_t)0xFFFFFFFFFFFF0000))) | ((v << 16) & (uint64_t)0xFFFFFFFFFFFF0000))

#define PICO_SIZE_GNADDRESS ((uint32_t)sizeof(struct pico_gn_address))
/// The GeoNetworking address that uniquely identifies a GeoNetworking entity.
PACKED_STRUCT_DEF pico_gn_address
{
    uint64_t value; ///< Contains all values for the GeoNetworking address. The first bit determines whether the address was manually set using \enum pico_gn_address_conf _method.AUTO, this means 0 when the address if manually configures, 1 if otherwise. The next five bits determine the type of the ITS-station. The next ten bits determine the ITS-station country code as described in 'ITU Operational Bulletin No. 741 - 1.VI.2001'. The last 48 bit represent the Logic Link Address.
};

#define PICO_SIZE_GNPOSITION (uint32_t)sizeof(struct pico_gn_position))
/// The target location for sending a GAC/GBC packet.
struct pico_gn_position
{
    
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
    struct pico_gn_address address; ///< The GeoNetworking address of which this \struct pico_gn_spv belongs to.
    uint32_t               timestamp; ///< The time in milliseconds at which the latitude and longitude were acuired.
    int32_t                latitude; ///< The latitude of the GeoAdhoc router reference position expressed in 1/10 micro degree.
    int32_t                longitude; ///< The longitude of the GeoAdhoc router reference position expresssed in 1/10 micro degree.
};

#define PICO_GET_GNLPV_ACCURACY(x) ((x >> 7) & 0x01)
#define PICO_GET_GNLPV_SPEED(x) (short_be(x) & 0x7FFF)
#define PICO_SET_GNLPV_ACCURACY(x, v) x = (((x & (~(uint16_t)0x80))) | ((v << 7) & 0x80))
#define PICO_SET_GNLPV_SPEED(x,v ) x = (((x & (~(uint16_t)0xFF7F))) | (short_be(v) & 0xFF7F))
#define PICO_SIZE_GNLPV ((uint32_t)sizeof(struct pico_gn_lpv))
/// The Long Position Vector containing all position-related information.
PACKED_STRUCT_DEF pico_gn_lpv
{
    struct pico_gn_spv short_pv; ///< The Short Position Vector containing the GeoNetworking address, timestamp, latitude and longitude.
    uint16_t           sac; ///< The frist bit indicates the accuracy of the reference position. The last 15 bit indicata the speed expressed in 0.01 metre per second.
    uint16_t           heading; ///< Heading expressed in 0.1 degree from North.
};

/// An union to be used when sending a GeoNetworking packet.
union pico_gn_destination
{
    struct pico_gn_position *position; ///< The position where the packet should be sent to. This field is used for GBC/GAC packets.
    struct pico_gn_address  *address; ///< The address where the packet should be sent to. This field is used for GUC packets.
};

#define PICO_GET_GNBASICHDR_VERSION(x) ((x & 0xF0) >> 4)
#define PICO_GET_GNBASICHDR_NEXT_HEADER(x) ((enum pico_gn_basic_header_next_header)(x & 0x0F))
#define PICO_SIZE_GNBASICHDR ((uint32_t)sizeof(struct pico_gn_basic_header))
/// The Basic Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_basic_header 
{
    uint8_t vnh; ///< The first four bits represent the version of the GeoNetworking protocol. The last four bits identifies the header immediately following the GeoNetworking Basic Header.
    uint8_t reserved; ///< Reserved, should be set to 0.
    uint8_t lifetime; ///< Indicates the maximum tolerable time a packet can be buffered until it reaches its destination.
    uint8_t remaining_hop_limit; ///< Decrembented by 1 by each GeoAdhoc router that forwards the packet. The packet shall not be forwarded if RHL is decremented to zero.
};

#define PICO_GET_GNTRAFFIC_CLASS_SCF(x) ((x->value >> 7) & 0x01)
#define PICO_GET_GNTRAFFIC_CLASS_CHANNEL_OFFLOAD(x) ((x->value >> 6) & 0x01)
#define PICO_GET_GNTRAFFIC_CLASS_ID(x) (x->value & 0x3F)
#define PICO_SIZE_GNTRAFFIC_CLASS (sizeof(struct pico_gn_traffic_class)))
/// The traffic class used for traffic classification which is used for particular mechanisms for data traffic management.
PACKED_STRUCT_DEF pico_gn_traffic_class
{
    uint8_t value; ///< The first bit identify the SCF-field. The next bit identify channel offload field. The last six bits identify the ID of the traffic class.
};

#define PICO_GET_GNCOMMONHDR_NEXT_HEADER(x) ((enum pico_gn_common_header_next_header)((x & 0xF0) >> 4))
#define PICO_GET_GNCOMMONHDR_HEADER(x) ((x & 0xF0) >> 4)
#define PICO_GET_GNCOMMONHDR_SUBHEADER(x) (x & 0x0F)
#define PICO_SIZE_GNCOMMONHDR ((uint32_t)sizeof(struct pico_gn_common_header))
/// The Common Header is a header present in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_common_header
{
    uint8_t                      next_header; ///< The first four bits identify the type of header immediately following the GeoNetworking headers. This value should be one of the following: PICO_GN_COMMON_HEADER_NEXT_HEADER_ANY, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_A, PICO_GN_COMMON_HEADER_NEXT_HEADER_BTP_B or PICO_GN_COMMON_HEADER_NEXT_HEADER_IPv6. The last four bits are reserved and must be set to 0.
    uint8_t                      header; ///< The first four bits identify the type of the GeoNetworking extended header The last four bits identify the sub-type of the GeoNetworking extended header.
    struct pico_gn_traffic_class traffic_class; ///< Traffic class that represents Facility-layer requirements on packet transport.
    uint8_t                      flags; ///< Bit 0: Indicates whether the ITS-S is mobile or stationary. Bit 1 to 7: Reserved, should be set to 0.
    uint16_t                     payload_length; ///< Length of the GeoNetworking payload, i.e. the rest of the packet following the whole GeoNetworking header in octets.
    uint8_t                      maximum_hop_limit; ///< Maximum hop limit, this value is NOT decremented by a GeoAdhoc router that forwards the packet.
    uint8_t                      reserved_2; ///< Reserved, should be set to 0.
};

#define PICO_SIZE_GNHDR ((uint32_t)sizeof(struct pico_gn_header))
/// The combined Basic Header and Common Header which are available in every GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_header
{
    struct pico_gn_basic_header  basic_header; ///< The Basic Header of the GeoNetworking packet.
    struct pico_gn_common_header common_header; ///< The Common Header of the GeoNetworking packet.
};

#define PICO_SIZE_GNDATA_REQUEST ((uint32_t)sizeof(struct pico_gn_data_request))
/// The GN-DATA.request primitive which contains information about the request for sending a GeoNetworking packet.
PACKED_STRUCT_DEF pico_gn_data_request
{
    uint8_t                            upper_proto; ///< The protocol responsible for this request. This value can be either PICO_PROTO_BTP_A, PICO_PROTO_BTP_B or PICO_PROTO_GN6ASL.
    uint8_t                            type; // The GeoNetworking packet type. 
    union pico_gn_destination         *destination; ///< The destination of this request.
    enum pico_gn_communication_profile communication_profile; ///< Specifies the underlying logic link protocol to use. This can either be unspecified (Ethernet for this implementation) ITS-G5 (NOT SUPPORTED).
    struct pico_gn_traffic_class       traffic_class; ///< The traffic class for this request.
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
int64_t pico_gn_find_extended_header_length(struct pico_gn_header *header);

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

/// Finds the \struct pico_gn_header_info, which contains extended header information, for a header.
///  \param header The header to find the extended header information for.
///  \returns The extended header information, NULL on failure.
const struct pico_gn_header_info *pico_gn_get_header_info(struct pico_gn_header *header);

/// Gets the next available sequence number
///  \returns The next sequence number.
uint16_t pico_gn_get_sequence_number(void);

/// Function for receiving the current time from the management interface.
///  \param result Sets the milliseconds elapsed since 2004-1-1 00:00:00.0000 modulo 2^32 inside the result.
///  \result 0 on success, -1 on failure.
int pico_gn_get_current_time(uint32_t *result);

/// Function for receiving the current position of the GeoAdhoc router from the management interface.
///  \param result Sets the \struct pico_gn_local_position_vector to the current position.
///  \result 0 on success, -1 on failure.
int pico_gn_get_position(struct pico_gn_local_position_vector *result);

/// Calculates the distance between two points using the haversine formula.
///  \param lat_a The latitude of point a.
///  \param long_a The longitude of point a.
///  \param lat_b The latitude of point b.
///  \param long_b The longitude of point b.
///  \returns The distance between a and b in kilometers.
double pico_gn_calculate_distance(int32_t lat_a, int32_t long_a, int32_t lat_b, int32_t long_b);

#endif	/* INCLUDE_PICO_GEONETWORKING */