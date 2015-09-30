/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_process
#define vidtk_frame_process

#include <process_framework/process.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <utilities/video_metadata.h>

namespace vidtk
{

template<class PixType>
class frame_process
  : public process
{
public:
  frame_process( vcl_string const& name, vcl_string const& class_name )
    : process( name, class_name ), has_roi_(false)
  {
  }

  virtual ~frame_process() {};

  virtual config_block params() const = 0;

  virtual bool set_params( config_block const& ) = 0;

  virtual bool initialize() = 0;

  /// \brief Proceed to the next image in the stream.
  ///
  /// Acquires an image so that subsequent calls to image() will
  /// return the next image in the stream.
  ///
  /// Note that a frame_process initially starts before the first
  /// frame.  This means that after initialization, the first call to
  /// step will result in the first frame.
  virtual bool step() = 0;

  /// \brief Seek to the given frame number.
  ///
  /// If successful (returns \c true(, sets up the stream so that the
  /// <emph>next call to step()</emph> will return the requested
  /// frame.
  virtual bool seek( unsigned frame_number ) = 0;

  virtual vidtk::timestamp timestamp() const = 0;

  virtual video_metadata const& metadata() const;

  virtual vil_image_view<PixType> const& image() const = 0;

  /// \brief The size of the images, if known.
  ///
  /// For image sequences of fixed size, ni() and nj() return the size
  /// of the image. It will return zero if the frame size is unknown a
  /// priori or is variable.
  virtual unsigned ni() const = 0;

  /// See ni().
  virtual unsigned nj() const = 0;

  ///Sets the the region one wants to have returned.  UNITS are in pixels
  bool set_roi(vcl_string & roi); //The roi must be in pixel units
  bool set_roi(unsigned int roi_x, unsigned int roi_y,
               unsigned int roi_width, unsigned int roi_height);
  void turn_off_roi();

  protected:
    bool has_roi_;
    unsigned int roi_x_, roi_y_, roi_width_, roi_height_;
    vidtk::video_metadata metadata_;
};


} // end namespace vidtk


#endif // vidtk_frame_process
