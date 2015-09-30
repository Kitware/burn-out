/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_list_writer_process_h_
#define vidtk_image_list_writer_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>
#include <boost/format.hpp>
#include <vcl_string.h>
#include <vil/vil_image_view.h>
#include <utilities/timestamp.h>
#include <vcl_fstream.h>


namespace vidtk
{


/// \brief Write images out to files.
///
/// The output file pattern, provided by the parameter \c pattern,
/// boost::format pattern.  The first argument will be the time
/// (double), the second the frame number (integer), and the third the
/// step count (integer).
///
/// See http://www.boost.org/libs/format/doc/format.html#examples
///
/// For example, "frame%2$05d.png" will write out files like
/// "frame00045.png", with 45 being the frame number.
///
/// If the parameter \c skip_unset_images is set to \c true, then
/// calls to step() without a corresponding call to set_image() will
/// silently succeed (without incrementing step count).  Otherwise, it
/// will fail.
///
template <class PixType>
class image_list_writer_process
  : public process
{
public:
  typedef image_list_writer_process self_type;

  image_list_writer_process( vcl_string const& name );

  ~image_list_writer_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_image( vil_image_view<PixType> const& img );

  VIDTK_INPUT_PORT( set_image, vil_image_view<PixType> const& );

  void set_timestamp( timestamp const& ts );

  VIDTK_INPUT_PORT( set_timestamp, timestamp const& );

  bool is_disabled() const;

protected:

  // parameters and internal variables

  config_block config_;

  bool disabled_;

  bool skip_unset_images_;

  boost::format pattern_;
  unsigned step_count_;

  vcl_string image_list_;

  vcl_ofstream list_out;

  // cached input

  vil_image_view<PixType> const* img_;
  timestamp ts_;
};


} // end namespace vidtk


#endif // vidtk_image_list_writer_process_h_
