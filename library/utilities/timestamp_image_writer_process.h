/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef INCL_TIMESTAMP_IMAGE_WRITER_PROCESS_H
#define INCL_TIMESTAMP_IMAGE_WRITER_PROCESS_H

#include <process_framework/process.h>
#include <process_framework/pipieline_aid.h>
#include <vil/vil_image_view.h>

namespace vidtk
{

class timestamp;

template < typename PixelType >
class timestamp_image_writer_process
  : public process
{
public:
  typedef timestamp_image_writer_process self_type;

  timestamp_image_process( const vcl_string& name );

  ~timestamp_image_process();

  virtual config_block params() const;

  virtual bool set_params( const config_block& );

  virtual bool initialize();

  virtual bool step();

  /// \brief Set the image to be timestamped
  void set_source_image( const vil_image_view< PixelType >& image );

  VIDTK_INPUT_PORT( set_source_image, const vil_image_view< PixelType >& );

  /// \brief Set the timestamp to be written to the image
  void set_source_timestamp( const timestamp& ts );

  VIDTK_INPUT_PORT( set_source_timestamp, const timestamp& );


  const vil_image_view< PixelType >& timestamped_image() const;

  VIDTK_OUTPUT_PORT( const vil_image_view< PixelType >&, timestamped_image );

protected:
  config_block config_;


  /// The images, non-stamped and timestamped
  vil_image_view< PixelType > unstamped_image_, timestamped_image_;

  /// The timestamp
  timestamp ts_;

};

} // end vidtk::

#endif
