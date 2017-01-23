/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_interface_h_
#define track_writer_interface_h_

#include <tracking_data/track.h>

#include <utilities/config_block.h>

#include <vbl/vbl_ref_count.h>
#include <vbl/vbl_smart_ptr.h>

#include <string>

namespace vidtk
{

// N.B. If you add a parameter here, you also need to add it to the
// config_ block in the track_writer_process.

  // name, type, initial-value
#define TRACK_WRITER_OPTION_LIST(CALL)                          \
  CALL( path_to_images, std::string, "")                        \
  CALL( frequency, double, 0 )                                  \
  CALL( x_offset, int, 0)                                       \
  CALL( y_offset, int, 0)                                       \
  CALL( write_lat_lon_for_world, bool, false )                  \
  CALL( write_utm, bool, false)                                 \
  CALL( suppress_header, bool, false)                           \
  CALL( classification, std::string, "UNCLASSIFIED")            \
  CALL( controlsystem, std::string, "NONE")                     \
  CALL( dissemination,  std::string, "FOUO")                    \
  CALL( policyname, std::string,  "Default")                    \
  CALL( releaseability, std::string,  "USA")                    \
  CALL( stanagVersion, std::string, "3.1")                      \
  CALL( nationality, std::string, "USA")                        \
  CALL( stationID, std::string, "E-SIM")                        \
  CALL( exerciseIndicator, std::string, "EXERCISE")             \
  CALL( simulationIndicator, std::string, "REAL")               \
  CALL( trackItemSource, std::string, "EO")                     \
  CALL( trackPointType, std::string, "AUTOMATIC ESTIMATED")     \
  CALL( flight_num, unsigned, 0)                                \
  CALL( name, std::string, "bd")                                \
  CALL( output_dir, std::string, "./")                          \
  CALL( cov_scale, double, 1.0)                                 \
  CALL( track_num_offset, unsigned, 0)                          \
  CALL( oracle_format, std::string, "")                         \
  CALL( db_type, std::string, "postgresql")                     \
  CALL( db_name, std::string, "perseas")                        \
  CALL( db_user, std::string, "perseas")                        \
  CALL( db_pass, std::string, "")                               \
  CALL( db_host, std::string, "localhost")                      \
  CALL( db_port, std::string, "12345")                          \
  CALL( connection_args, std::string, "")                       \
  CALL( processing_mode, std::string, "BATCH_MODE")             \
  CALL(write_image_chip_disabled, bool, false)                  \
  CALL(write_image_data_mask_mode, std::string, "ALWAYS")       \
  CALL(aoi_id, vxl_int_32, -1)

// ----------------------------------------------------------------
/*! \brief Track writer options.
 *
 * This class contains the options for all track writers.  These
 * options are a superset of all track writer options because when the
 * options are specified it is not always known which track writer is
 * going to be instantiated.
 *
 * The actual fields in this class is driven by the macroized list.
 */
class track_writer_options
{
public:
  track_writer_options()
  {
#define INIT(N,T,I) N ## _ = I;
    TRACK_WRITER_OPTION_LIST(INIT)
#undef INIT
  }

#define ALLOC(N,T,I) T N ## _;
  TRACK_WRITER_OPTION_LIST(ALLOC)
#undef ALLOC
};


class track_writer_interface;

/// Create name for smart pointer
typedef vbl_smart_ptr<track_writer_interface> track_writer_interface_sptr;

// ----------------------------------------------------------------
/*! \brief Track writer interface class.
 *
 * This class represents the interface for track writers. Each
 * concrete class must inherit from this abstract base class and
 * implement this interface.
 */
class track_writer_interface
  : public vbl_ref_count
{
  public:
    track_writer_interface( );
    virtual ~track_writer_interface();

    virtual bool write( track_sptr const track );
    virtual bool write( image_object_sptr const io, timestamp const& ts );

    virtual void set_options(track_writer_options const& options) = 0;

    virtual bool write( track_vector_t const& tracks );
    virtual bool write( std::vector<vidtk::image_object_sptr> const& ios, timestamp const& ts );
    virtual bool write( track_vector_t const& tracks,
                        std::vector<vidtk::image_object_sptr> const* ios,
                        timestamp const& ts );

    virtual bool write( vidtk::track const& track ) = 0;
    virtual bool write( image_object const& io, timestamp const& ts);

    virtual bool open( std::string const& fname ) = 0;
    virtual bool is_open() const = 0;
    virtual void close() = 0;
    virtual bool is_good() const = 0;

};

} // end namespace vidtk

#endif
