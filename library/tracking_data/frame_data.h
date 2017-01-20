/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_frame_data_
#define vidtk_frame_data_

#include <vil/vil_image_view.h>
#include <vgl/vgl_box_2d.h>

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/shot_break_flags.h>
#include <tracking_data/track.h>

#include <utilities/video_metadata.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>

#include <boost/shared_ptr.hpp>

namespace vidtk
{

/// \brief A structure for encapsulating lots of information about the current frame.
///
/// This structure is currently only used for descriptor generators for accessing
/// frame contents and related information, but in the future it could be used elsewhere
/// as well, such as in the tracker. Not all quanities need be set, only those used
/// by downstream elements.
class frame_data
{

public:

  /// Frame Data Constructor
  frame_data();

  /// Frame Data Deconstructor
  virtual ~frame_data();

  /// Set inputs. Only inputs which get used need to be set. If something attempts
  /// to use a quantity that was not provided, an error will be thrown.
  void set_active_tracks( track::vector_t const& tracks );
  void set_metadata( video_metadata const& meta );
  void set_timestamp( timestamp const& ts );
  void set_image_to_world_homography( homography::transform_t const& homog );
  void set_modality( video_modality modality );
  void set_shot_break_flags( shot_break_flags sb_flags );
  void set_gsd( double gsd );

  /// Set an input image and automatically determine it's type.
  template< typename PixType >
  void set_image( vil_image_view< PixType > const& img )
  {
    set_internal_image( img );
  }

  /// Manually specify different types of input images.
  void set_image_8u( vil_image_view< vxl_byte > const& img );
  void set_image_16u( vil_image_view< vxl_uint_16 > const& img );
  void set_image_8u_gray( vil_image_view< vxl_byte > const& img );
  void set_image_16u_gray( vil_image_view< vxl_uint_16 > const& img );

  /// Access internal data.
  vidtk::track::vector_t const& active_tracks() const;
  vidtk::video_metadata const& metadata() const;
  vidtk::timestamp const& frame_timestamp() const;
  vidtk::homography::transform_t const& world_homography() const;
  vidtk::video_modality const& modality() const;
  vidtk::shot_break_flags const& shot_breaks() const;
  unsigned int frame_number() const;
  double frame_time() const;
  double average_gsd() const;

  /// Retrieve a color image of the specified type if one is available,
  /// or can be computed from the inputs which we have. If it can't, a
  /// single channel image is returned, followed by no image at all.
  template< typename PixType >
  vil_image_view< PixType > const& image() const
  {
    return internal_image( PixType() );
  }

  /// Manually request an image of a specified type.
  vil_image_view< vxl_byte > const& image_8u();
  vil_image_view< vxl_uint_16 > const& image_16u();
  vil_image_view< vxl_byte > const& image_8u_gray();
  vil_image_view< vxl_uint_16 > const& image_16u_gray();

  /// Check if certain inputs were ever set
  bool was_image_set() const { return was_any_image_set_; }
  bool was_tracks_set() const { return was_tracks_set_; }
  bool was_metadata_set() const { return was_metadata_set_; }
  bool was_gsd_set() const { return was_gsd_set_; }
  bool was_world_homography_set() const { return was_homography_set_; }
  bool was_modality_set() const { return was_modality_set_; }
  bool was_break_flags_set() const { return was_sb_set_; }
  bool was_timestamp_set() const { return was_timestamp_set_; }

  /// Other helper functions

  /// Perform a deep copy of another buffer and all of its contents
  void deep_copy( const frame_data& other );

protected:

  /// Reset all is_set flags
  void reset_set_flags();

private:

  // Contents for this frame
  vidtk::track::vector_t active_tracks_;
  vidtk::video_metadata metadata_;
  vidtk::timestamp timestamp_;
  vidtk::homography::transform_t world_homog_;
  vidtk::video_modality modality_;
  vidtk::shot_break_flags sb_flags_;
  double gsd_;

  // Remember which inputs were ever set
  bool was_tracks_set_;
  bool was_metadata_set_;
  bool was_timestamp_set_;
  bool was_homography_set_;
  bool was_modality_set_;
  bool was_gsd_set_;
  bool was_sb_set_;

  // Imagery related internal variables and functions
  bool was_any_image_set_;

  void set_internal_image( const vil_image_view< vxl_byte >& img );
  void set_internal_image( const vil_image_view< vxl_uint_16 >& img );

  vil_image_view< vxl_byte > const& internal_image( vxl_byte ) const;
  vil_image_view< vxl_uint_16 > const& internal_image( vxl_uint_16 ) const;

  class image_data;
  boost::shared_ptr< image_data > image_data_;
};

// Frame data smart pointer declaration
typedef boost::shared_ptr< frame_data > frame_data_sptr;

} // end namespace vidtk

#endif
