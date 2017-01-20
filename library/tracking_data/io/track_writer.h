/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef track_writer_h_
#define track_writer_h_

#include <tracking_data/track.h>
#include <tracking_data/io/track_writer_interface.h>
#include <utilities/config_block.h>

#include <string>
#include <map>

#include <boost/noncopyable.hpp>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Track writer.
 *
 * This class represents a track writer that can write tracks and
 * associated data in various formats. Individual track output formats
 * are implemented by deriving a class from track_writer_interface.
 * Common utput formats are registered by default in the
 * constructor. Custom formats can be registered at run time.
 *
 * \sa track_writer_interface
 *
 * Dynamics
 \msc
 process, writer, format;

 process=>writer [ label = "<<create>>" ];
 writer=>writer [ label = "add_writer()" ];
 --- [ label = "for all file formats" ];

process=>writer [ label = "add_writer() - add custom writer" ];
process=>writer [ label = "get_writer_names() - for help message" ];

 --- [ label = "process::set_params()" ];
process=>writer [ label = "set_format()" ];
writer=>writer [ label = "get_writer()" ];

 --- [ label = "process::initialize()" ];
process=>writer [ label = "open()" ];

 --- [ label = "process::step2()" ];
process=>writer [ label = "write()" ];

 \endmsc
 *
 */
class track_writer
  : private boost::noncopyable
{
public:
  track_writer( );

  ~track_writer();

  bool write( track_vector_t const& tracks );
  bool write( track_vector_t const& tracks,
              std::vector< vidtk::image_object_sptr > const* ios,
              timestamp const& ts );

  /**
   * \brief Select output format.
   *
   * This method selects the output format that is to be written. The
   * format name must be one that is currently supported. (\sa
   * get_writer_names()) The format must be sepected before the file
   * is opened.
   *
   * @param format Name of format to write
   *
   * @return \b true if format is registered; \b false otherwise.
   */
  bool set_format( std::string const& format );


  /**
   * \brief Open names file for writing.
   *
   * The named file is opened, and created if necessary.
   *
   * @param fname Name of file to open.
   *
   * @return \b true if file is opened; \b false otherwise.
   */
  bool open( std::string const& fname );

  bool is_open() const;
  void close();
  bool is_good() const;
  void set_options(track_writer_options const& options);
  void add_writer(std::string const& tag, track_writer_interface_sptr w);


  /**
   * \brief Get list of file format names.
   *
   * This method returns a list of all currently registered file
   * format writer names (or format types). This string can be used as
   * the config parameter description for the format to write.
   *
   * @return List of file format names.
   */
  std::string const& get_writer_formats() const;
  config_block const& get_writer_config() const;

private:
  track_writer_interface_sptr get_writer(std::string const& tag);
  track_writer_interface_sptr writer_;
  std::map< std::string, track_writer_interface_sptr > writer_options_;
  std::string writer_formats_;
  config_block m_plugin_config;
};

} //namespace vidtk

#endif
