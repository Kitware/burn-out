/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_kw18_timestamp_parser_h_
#define vidtk_kw18_timestamp_parser_h_

#include <utilities/timestamp.h>

#include <string>
#include <queue>
#include <vector>
#include <set>

namespace vidtk
{

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard set.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::set< vidtk::timestamp >& output );

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard queue.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::queue< vidtk::timestamp >& output );

/// Given a kw18/20 track file, parse all of the timestamps in it
/// and put them into a standard vector.
bool load_timestamps_from_kw18( const std::string& filename,
                                std::vector< vidtk::timestamp >& output );


} // end namespace vidtk

#endif // vidtk_kw18_timestamp_parser_h_
