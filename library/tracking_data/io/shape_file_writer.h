/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef shape_file_writer_h_
#define shape_file_writer_h_

#include <tracking_data/track.h>
#include <utilities/geo_coordinate.h>
#include <string>

#include <boost/noncopyable.hpp>


// This relies on shapelib (http://shapelib.maptools.org/)

namespace vidtk
{

class shape_file_writer
  : private boost::noncopyable
{
  public:
    shape_file_writer( bool apix_way = true );
    ~shape_file_writer();
    bool write( vidtk::track_sptr track );
    bool write( vidtk::track const & track );
    void open( std::string & fname );
    void close();
    void set_shape_type( int shape_type );
    void set_is_utm(bool v);
    void set_utm_zone(int zone, bool is_north);

    /**
     * @brief Reformats time.
     *
     * \todo A description of how this works and why it is part of this
     * class rather than a stand-alone unbound function.
     *
     * @param time_format_string
     * @param t_in_s
     * @param utc_time
     * @param utc_time_ms
     * @param time_string
     */
    static void reformat_time( std::string const& time_format_string,
                               double t_in_s,
                               long& utc_time,
                               int& utc_time_ms,
                               std::string& time_string );

private:
    struct internal;
    internal * internal_;
    int shape_type_;
    bool is_open_;
    bool is_utm_;
    int zone_;
    bool is_north_;
    unsigned int count_;
    bool apix_way_;
    bool write_old( vidtk::track const& track );
    bool write_apix( vidtk::track const& track );
    geo_coord::geo_coordinate_sptr get_lat_lon( vidtk::track_state_sptr track );

}; // class shape_file_writer

} //namespace vidtk

vidtk::shape_file_writer & operator<<( vidtk::shape_file_writer & writer, vidtk::track_sptr );

#endif
