/*ckwg +5
 * Copyright 2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef _VIDTK_TRACK_READER_DB_H_
#define _VIDTK_TRACK_READER_DB_H_

#include <database/connection/db_connection_factory.h>
#include <database/track_db_interface.h>

#include <tracking_data/io/track_reader_interface.h>
#include <string>
#include <ostream>

namespace vidtk {
namespace ns_track_reader {

class track_reader_db : public track_reader_interface
{
public:
  track_reader_db();
  virtual ~track_reader_db();

  virtual bool open( std::string const& filename );

  virtual bool read_next_terminated( vidtk::track::vector_t&  datum,
                                     unsigned&                frame );

  virtual size_t read_all( vidtk::track::vector_t& datum );

protected:
  db_connection_sptr   db_conn_;
  track_db_sptr        track_db_;

  bool is_connected_;

}; // end class track_reader_db

} // end namespace
} // end namespace

#endif /* _VIDTK_TRACK_READER_DB_H_ */
