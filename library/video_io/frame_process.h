/*ckwg +5
 * Copyright 2010-2016 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_process_h_
#define vidtk_frame_process_h_

#include <process_framework/process.h>
#include <utilities/timestamp.h>
#include <vil/vil_image_view.h>
#include <utilities/video_metadata.h>

namespace vidtk
{

// ----------------------------------------------------------------
/*! \brief Abstract base class for video source.
 *
 * This class is the abstract base class for all video source
 * processes.
 */
template<class PixType>
class frame_process
  : public process
{
public:
  frame_process( std::string const& _name, std::string const& _class_name )
    : process( _name, _class_name ), has_roi_(false)
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
  virtual bool step() { return false; };

  /// \brief Seek to the given frame number.
  ///
  /// \param frame_number - Frame number to seek to.
  ///
  /// \return If successful (returns \c true ), sets up the stream so
  /// that the <b>next call to step()</b> will return the requested
  /// frame.
  virtual bool seek( unsigned frame_number ) = 0;

  /// \brief The total number of frames, if known.
  ///
  /// \return The number of frames in the image sequence, if known, otherwise
  /// \c 0.
  virtual unsigned nframes() const = 0;

  /// \brief The frame rate, if known.
  ///
  /// \return The frame rate of the image sequence (in frames per second), if
  /// known, otherwise \c 0.0.
  virtual double frame_rate() const = 0;

  // Accessors / output port methods
  virtual vidtk::timestamp timestamp() const = 0;
  virtual video_metadata metadata() const;
  virtual vil_image_view<PixType> image() const = 0;

  /// \brief The size of the images, if known.
  ///
  /// For image sequences of fixed size, ni() and nj() return the size
  /// of the image. It will return zero if the frame size is unknown a
  /// priori or is variable.
  virtual unsigned ni() const = 0;

  /// See ni().
  virtual unsigned nj() const = 0;

  /// Sets the the region one wants to have returned.  UNITS are in pixels
  virtual bool set_roi(std::string & roi); // The roi must be in pixel units
  virtual bool set_roi(unsigned int roi_x, unsigned int roi_y,
               unsigned int roi_width, unsigned int roi_height);
  void turn_off_roi();

  ///@todo Add capabilities methods that report on the features
  /// supplied by the derived classes.
  /// Candidates are:
  /// bool has_seek()
  /// bool has_metadata()
  /// Note that there are at least two metadata states that can be reported.
  /// 1) process supports metadata
  /// 2) current video contains metadata

protected:
  bool has_roi_; ///< an roi has been specified
  unsigned int roi_x_, roi_y_, roi_width_, roi_height_; ///< ROI specification
  vidtk::video_metadata metadata_; ///< metadata for current frame (if available)
};


} // end namespace vidtk


#endif // vidtk_frame_process_h_
