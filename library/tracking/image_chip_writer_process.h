/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_chip_writer_process_h_
#define vidtk_image_chip_writer_process_h_

// track_writer_process includes
#include <vcl_fstream.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <utilities/timestamp.h>

#include <tracking/vsl/image_object_io.h>
#include <tracking/image_object.h>
#include <tracking/track.h>
#include <tracking/tracking_keys.h>
// end track_writer_process includes

#include <vil/vil_image_view.h>

typedef vil_image_view<vxl_byte> iChip_view;
typedef vil_image_view<bool> mChip_view;


namespace vidtk
{

class image_chip_writer_process
  : public process 
{
  
public:
  typedef image_chip_writer_process self_type;

  image_chip_writer_process( vcl_string const& name );

  ~image_chip_writer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_tracks( vcl_vector< track_sptr > const& trks );

  VIDTK_INPUT_PORT( set_tracks, vcl_vector< track_sptr > const& );
  
  bool is_image_chip_disabled() const;

  bool is_alpha_chip_disabled() const;

private:

  bool fail_if_exists( vcl_string const& filename );
  void write_image_chip(vcl_string const& filename, iChip_view const& imageChip);
  void write_alpha_chip(vcl_string const& filename, iChip_view const& alphaChip);

  // Parameters

  config_block config_;
  bool image_chip_disabled_;
  bool alpha_chip_disabled_;
  vcl_string chip_location_;
  vcl_string image_chip_filename_format_;
  vcl_string alpha_chip_filename_format_;
  bool allow_overwrite_;


  // Cached input

  vcl_vector< track_sptr > const* src_trks_;

  //timestamp derivation parameter
  double frequency_;

};

} // End namespace vidtk

#endif //vidtk_image_chip_writer_process_h_
