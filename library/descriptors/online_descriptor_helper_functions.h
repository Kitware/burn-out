/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_online_descriptor_generator_helpers_h_
#define vidtk_online_descriptor_generator_helpers_h_

#include <tracking_data/raw_descriptor.h>
#include <tracking_data/frame_data.h>

#include <vil/vil_image_view.h>
#include <vil/vil_crop.h>
#include <vil/vil_convert.h>
#include <vgl/vgl_box_2d.h>
#include <vgl/vgl_intersection.h>
#include <vnl/vnl_double_3x3.h>
#include <vnl/vnl_vector_fixed.h>
#include <vnl/vnl_double_2.h>
#include <vnl/vnl_double_3.h>

#include <tracking_data/shot_break_flags.h>
#include <tracking_data/track.h>

#include <utilities/video_metadata.h>
#include <utilities/video_modality.h>
#include <utilities/homography.h>
#include <utilities/point_view_to_region.h>

#include <queue>
#include <vector>
#include <stdexcept>

namespace vidtk
{


/**
 * \brief Raw descriptor factory.
 *
 * This method creates a new raw descriptor with the supplied
 * name. This factory method ensures that the new descriptor is
 * managed with a smart pointer.
 *
 * @param type Name for the new descriptor.
 *
 * @return A smart pointer to the new descriptor is returned.
 */
descriptor_sptr create_descriptor( std::string const& type );


/**
 * \brief Raw descriptor factory.
 *
 * This method creates a new raw descriptor with the supplied name an
 * track id. This factory method ensures that the new descriptor is
 * managed with a smart pointer.
 *
 * @param type Name for the new descriptor.
 * @param track_id Track ID to add to descriptor.
 *
 * @return A smart pointer to the new descriptor is returned.
 */
descriptor_sptr create_descriptor( std::string const& type, track::track_id_t track_id );


/**
 * \brief Raw descriptor factory.
 *
 * This method creates a new raw descriptor with the supplied name and
 * track id, taken from supplied track.
 *
 * This factory method ensures that the new descriptor is
 * managed with a smart pointer.
 *
 * @param type Name for the new descriptor.
 * @param track Track to supply track ID.
 *
 * @return A smart pointer to the new descriptor is returned.
 */
descriptor_sptr create_descriptor( std::string const& type, const track_sptr &track );


/**
 * \brief Normalize the feature vector contained within the descriptor.
 *
 * @param ptr A pointer to the descriptor
 *
 * @return Was the descriptor successfully normalized?
 */
bool normalize_descriptor( const descriptor_sptr& ptr );


/**
 * \brief Copy auxialiary descriptor information between descriptors.
 *
 * Copies any track id references and history information from one
 * descriptor to another. Note: doesn't copy any descriptor id info
 * or features vectors, only history and track id references.
 *
 * @param from_ptr A pointer to the input descriptor
 * @param to_ptr A pointer to the output descriptor
 */
void copy_aux_descriptor_info( const descriptor_sptr& from_ptr,
                               descriptor_sptr& to_ptr );


/**
 * \brief Scale an image region by some factor.
 *
 * @param scale Scale factor (multiplication ratio)
 * @param shift Shift factor (number of pixels)
 */
void scale_region( image_region& bbox, double scale, unsigned shift = 0 );


/**
 * \brief Scale an image region by some factor.
 *
 * @param scale Scale factor (multiplication ratio)
 * @param shift Shift factor (number of pixels)
 */
void scale_region( vgl_box_2d< int >& bbox, double scale, unsigned shift = 0 );


/**
 * \brief Transform a 2D point using an homography.
 *
 * Transform a point from source to destination using the given homography
 *
 * @param src Input point
 * @param H_src2dst Input homography
 *
 * @return Transformed point
 */
template< typename FloatType >
vgl_point_2d<FloatType> transform_point( const vgl_point_2d<FloatType>& src,
                                         vnl_double_3x3 const& H_src2dst )
{
  vnl_double_3 sp( src.x(), src.y(), 1.0 );
  vnl_double_3 dp = H_src2dst * sp;
  return vgl_point_2d< FloatType >( dp[0] / dp[2], dp[1] / dp[2] );
}


/**
 * \brief Return the last known tracking mask for this track.
 *
 * @param track Input track
 *
 * @return Either an empty image, or the retrieved mask
 */
vil_image_view<bool> get_tracking_mask( track_sptr const& track );


/**
 * \brief Dump the contents of some image to a feature vector
 *
 * @param image Input image
 * @param ptr Descriptor to dump feature vector into
 */
template< typename PixType >
void dump_image_to_descriptor( const vil_image_view< PixType >& image, descriptor_sptr& ptr )
{
  // Get input properties
  unsigned ni = image.ni(), nj = image.nj(), np = image.nplanes();

  // Deep copy into output descriptor (interlacing channels)
  descriptor_data_t& output = ptr->get_features();
  output.resize( image.size() );
  vil_image_view<double> dest( &output[0], ni, nj, np, np, nj * np, 1 );

  // Convert cast will not deep copy if the types are the same
  if( vil_pixel_format_of(double()) == vil_pixel_format_of(PixType()) )
  {
    dest.deep_copy( image );
  }
  else
  {
    vil_convert_cast( image, dest );
  }
}


/**
 * \brief Return the last known bounding box for this track.
 *
 * @param track Input track
 *
 * @return The latest bounding box contained in the track
 */
vgl_box_2d< unsigned > get_latest_bbox( const track_sptr& track );


/**
 * \brief Generate a descriptor history entry for the given track.
 *
 * Generates a recommended descriptor history entry for this frame,
 * with as many entries filled out as we can given our inputs
 *
 * @param frame Input frame
 * @param track Input track
 *
 * @return Potential history entry
 */
descriptor_history_entry generate_history_entry( const frame_data_sptr& frame,
                                                 const track_sptr& track );


/**
 * \brief Convert one type of bounding box to another
 *
 * @param box1 Input box
 * @param box2 Output box
 */
template< typename T1, typename T2 >
void
convert_cast_bbox( const vgl_box_2d<T1>& box1, vgl_box_2d<T2>& box2 )
{
  box2.set_min_x( static_cast<T2>( box1.min_x() ) );
  box2.set_max_x( static_cast<T2>( box1.max_x() ) );
  box2.set_min_y( static_cast<T2>( box1.min_y() ) );
  box2.set_max_y( static_cast<T2>( box1.max_y() ) );
}


/**
 * \brief Sum the contents of some vector with those stored in a descriptor
 *
 * If the descriptor feature vector is not the same size as the input vector
 * then it will be reset.
 *
 * @param vec Input vector
 * @param desc_ptr Descriptor
 */
template< typename T1 >
void
add_features_into( const std::vector< T1 > vec, descriptor_sptr desc_ptr )
{
  if( !desc_ptr )
  {
    return;
  }

  descriptor_data_t& desc_data = desc_ptr->get_features();

  if( desc_data.size() != vec.size() )
  {
    desc_data = descriptor_data_t( vec.begin(), vec.end() );
    return;
  }

  for( unsigned i = 0; i < vec.size(); ++i )
  {
    desc_data[i] += vec[i];
  }
}


/**
 * \brief Take the maximum of the values stored in the descriptor and some input
 *
 * If the descriptor feature vector is not the same size as the input vector
 * then it will be reset to the input vector.
 *
 * @param vec Input vector
 * @param desc_ptr Descriptor
 */
template< typename T1 >
void
take_max_features( const std::vector< T1 > vec, descriptor_sptr desc_ptr )
{
  if( !desc_ptr )
  {
    return;
  }

  descriptor_data_t& desc_data = desc_ptr->get_features();

  if( desc_data.size() != vec.size() )
  {
    desc_data = descriptor_data_t( vec.begin(), vec.end() );
    return;
  }

  for( unsigned i = 0; i < vec.size(); ++i )
  {
    desc_data[i] = std::max( desc_data[i], static_cast< double >( vec[i] ) );
  }
}


/**
 * \brief Add the contents of some vector to the end of a descriptor
 *
 * @param vec Input vector
 * @param desc_ptr Descriptor
 */
template< typename T1 >
void
append_features_onto( const std::vector< T1 > vec, descriptor_sptr desc_ptr )
{
  if( !desc_ptr )
  {
    return;
  }

  descriptor_data_t& desc_data = desc_ptr->get_features();

  desc_data.insert( desc_data.end(), vec.begin(), vec.end() );
}


} // end namespace vidtk

#endif
