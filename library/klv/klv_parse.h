/*ckwg +5
 * Copyright 2012-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_KLV_PARSE_H_
#define VIDTK_KLV_PARSE_H_

#include <vxl_config.h>
#include <vector>
#include <deque>
#include <ostream>

#include <klv/klv_key.h>


namespace vidtk
{

class klv_data;

/// Define a type for KLV LDS key-value pairs
typedef std::pair<klv_lds_key, std::vector<vxl_byte> > klv_lds_pair;
typedef std::vector< klv_lds_pair > klv_lds_vector_t;

/// Define a type for KLV UDS key-value pairs
typedef std::pair<klv_uds_key, std::vector<vxl_byte> > klv_uds_pair;
typedef std::vector< klv_uds_pair > klv_uds_vector_t;

/** Pop the first KLV UDS key-value pair found in the data buffer.
 *
 * The first valid KLV packet found in the data stream is returned.
 * Leading bytes that do not belong to a KLV pair are dropped.
 *
 * @param[in,out] data Byte stream to be parsed.
 * @param[out] klv_packet Full klv packet with key and data fields
 * specified.
 *
 * @return \c true if packet returned; \c false if no packet returned.
 */
bool klv_pop_next_packet( std::deque< vxl_byte >& data, klv_data& klv_packet);


/** Parse KLV LDS (Local Data Set) from an array of bytes The input
 * array is the raw KLV packet. The output is a vector of LDS
 * packets.
 *
 * @param[in] data KLV raw packet
 *
 * @return A vector of klv LDS packets.
 */
klv_lds_vector_t
parse_klv_lds(klv_data const& data);

/** Parse KLV UDS (Universal Data Set) from an array of bytes The input
 * array is the raw KLV packet. The output is a vector of UDS
 * packets.
 *
 * @param[in] data KLV raw packet
 *
 * @return A vector of klv UDS packets.
 */
klv_uds_vector_t
parse_klv_uds( klv_data const& data );


/** \brief Print KLV packet.
 * The supplied KLV packet is decoded and printed.
 *
 * @param str - stream to format on
 * @param klv - packet to decode
 */
std::ostream &
print_klv( std::ostream & str, klv_data const& klv );

} // end namespace vidtk


#endif // VIDTK_KLV_PARSE_H_
