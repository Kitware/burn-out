/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "kmeans_segmentation.h"

#include <logger/logger.h>

#include <algorithm>
#include <limits>
#include <map>

#include <vil/vil_transform.h>

#ifdef USE_OPENCV

#include <opencv/cxcore.h>
#include <opencv/cv.h>

#elif USE_MUL

#include <mbl/mbl_data_array_wrapper.h>
#include <vnl/vnl_vector.h>

#endif

#include <boost/bind.hpp>

namespace vidtk
{

VIDTK_LOGGER( "kmeans_segmentation" );


// Unoptimized function for the general case of finding the closest
// value for a given point to some list of centers via L2 distance.
template < typename PixType, typename AccumType, typename LabelType >
inline LabelType calculate_label_l2( const PixType *pos,
                                     const std::ptrdiff_t pstep,
                                     const unsigned np,
                                     const PixType *centers,
                                     const unsigned nc )
{
  const PixType* end_pos = pos + pstep * np;
  const PixType* input_itr;

  AccumType min_dist = std::numeric_limits<AccumType>::max();
  LabelType ind = 0;

  for( unsigned c = 0; c < nc; c++ )
  {
    AccumType computed_dist = 0;
    for( input_itr = pos; input_itr < end_pos; centers++, input_itr += pstep )
    {
      AccumType diff = static_cast<AccumType>(*centers) -
                       static_cast<AccumType>(*input_itr);

      computed_dist += ( diff * diff );
    }
    if( computed_dist < min_dist )
    {
      min_dist = computed_dist;
      ind = c;
    }
  }
  return ind;
}

// Precompute an index for every possible value for the optimized case
template < typename PixType, typename LabelType >
void initialize_precomputed_index_1d( const std::vector< PixType >& centers,
                                      std::vector< LabelType >& index )
{
  LOG_ASSERT( centers.size() > 0, "Must have at least 1 center specified" );

  index.resize( static_cast<unsigned>(std::numeric_limits< PixType >::max())+1 );

  for( unsigned i = 0; i < index.size(); i++ )
  {
    const PixType value = static_cast<PixType>( i );
    index[i] = calculate_label_l2<PixType,double,LabelType>( &value, 1, 1u, &centers[0], centers.size() );
  }
}

template < typename PixType, typename LabelType >
LabelType label_from_index_1d( const PixType& pixel, LabelType* index )
{
  return index[ pixel ];
}

// General backup unoptimized centers => index mapping method
template < typename PixType, typename AccumType, typename LabelType >
void label_image_from_centers_unopt( const vil_image_view< PixType >& src,
                                     const std::vector< PixType >& centers,
                                     const unsigned center_count,
                                     vil_image_view< LabelType >& dst )
{
  const unsigned ni = src.ni(), nj = src.nj(), np = src.nplanes();

  const std::ptrdiff_t sistep = src.istep();
  const std::ptrdiff_t sjstep = src.jstep();
  const std::ptrdiff_t spstep = src.planestep();

  const std::ptrdiff_t distep = dst.istep();
  const std::ptrdiff_t djstep = dst.jstep();

  const PixType* srow = src.top_left_ptr();
  const PixType* cstart = &centers[0];
  LabelType* drow = dst.top_left_ptr();

  for( unsigned j = 0; j<nj; j++, srow += sjstep, drow += djstep )
  {
    const PixType* scol = srow;
    LabelType* dcol = drow;

    for( unsigned i=0; i<ni; i++, scol += sistep, dcol += distep )
    {
      *dcol = calculate_label_l2<PixType, AccumType, LabelType>( scol, spstep, np, cstart, center_count );
    }
  }
}

template < typename PixType, typename LabelType >
void label_image_from_centers_md( const vil_image_view< PixType >& src,
                                  const std::vector< PixType >& centers,
                                  const unsigned center_count,
                                  vil_image_view< LabelType >& dst )
{
  label_image_from_centers_unopt< PixType, double, LabelType >( src, centers, center_count, dst );
}

// Use integer opts for byte type
template < typename LabelType >
void label_image_from_centers_md( const vil_image_view< vxl_byte >& src,
                                  const std::vector< vxl_byte >& centers,
                                  const unsigned center_count,
                                  vil_image_view< LabelType >& dst )
{
  label_image_from_centers_unopt< vxl_byte, int, LabelType >( src, centers, center_count, dst );
}

// Specializations of the below function for a single plane type
template < typename PixType, typename LabelType >
void label_image_from_centers_1d( const vil_image_view< PixType >& src,
                                  const std::vector< PixType >& centers,
                                  vil_image_view< LabelType >& dst )
{
  label_image_from_centers_md( src, centers, 1, dst );
}

template < typename LabelType >
void label_image_from_centers_1d( const vil_image_view< vxl_byte >& src,
                                  const std::vector< vxl_byte >& centers,
                                  vil_image_view< LabelType >& dst )
{
  std::vector< LabelType > index;
  initialize_precomputed_index_1d( centers, index );

  vil_transform( src,
                 dst,
                 boost::bind( label_from_index_1d<vxl_byte, LabelType>, _1, &index[0] ) );
}

template < typename LabelType >
void label_image_from_centers_1d( const vil_image_view< vxl_uint_16 >& src,
                                  const std::vector< vxl_uint_16 >& centers,
                                  vil_image_view< LabelType >& dst )
{
  std::vector< LabelType > index;
  initialize_precomputed_index_1d( centers, index );

  vil_transform( src,
                 dst,
                 boost::bind( label_from_index_1d<vxl_uint_16, LabelType>, _1, &index[0] ) );
}

template < typename PixType, typename LabelType >
void label_image_from_centers( const vil_image_view< PixType >& src,
                               const std::vector< std::vector< PixType > >& centers,
                               vil_image_view< LabelType >& dst )
{
  LOG_ASSERT( centers.size() > 0, "Inputs must have at least 1 center specified" );
  LOG_ASSERT( centers[0].size() == src.nplanes(), "Center dims do not match image size" );

  dst.set_size( src.ni(), src.nj() );

  // Ensure centers vector is stored in a contiguous vector
  std::vector< PixType > truncated_centers;
  for( unsigned i = 0; i < centers.size(); i++ )
  {
    truncated_centers.insert( truncated_centers.end(),
                              centers[i].begin(),
                              centers[i].end() );
  }

  // Choose optimization level (in the 1 dimensional low integral case, preindex an array
  // of pre-computed labels for all possible values).
  if( src.nplanes() == 1 )
  {
    label_image_from_centers_1d( src, truncated_centers, dst );
  }
  // Unoptimized general case
  else
  {
    label_image_from_centers_md( src, truncated_centers, centers.size(), dst );
  }
}

#ifdef USE_OPENCV

// Cluster 'sample_points' points from image 'img' into 'clusters' clusters.
template< typename PixType >
void ocv_cluster( const vil_image_view<PixType>& image,
                  const unsigned clusters,
                  const unsigned desired_sample_points,
                  std::vector< std::vector<PixType> >& centers )
{
  centers.clear();

  const unsigned nplanes = image.nplanes();
  const unsigned image_pixels = image.ni() * image.nj();
  const unsigned pts_to_sample = std::min( desired_sample_points, image_pixels );
  const unsigned sample_step = image_pixels / pts_to_sample;

  const std::ptrdiff_t pstep = image.planestep();

  cv::Mat samples_mat( pts_to_sample, nplanes, CV_32F );
  cv::Mat labels_mat;
  cv::Mat centers_mat;

  // Sample image points at different locations within the image
  unsigned pos = 0;

  for( unsigned pt = 0; pt < pts_to_sample; pt++, pos += sample_step )
  {
    unsigned si = pos % image.ni();
    unsigned sj = ( pos / image.ni() ) % image.nj();

    const PixType *ptr = &image(si,sj,0);
    samples_mat.at<float>( pt, 0 ) = *ptr;

    for( unsigned plane = 1; plane < nplanes; plane++ )
    {
      ptr += pstep;
      samples_mat.at<float>( pt, plane ) = *ptr;
    }
  }

  // Perform kmeans
  cv::kmeans( samples_mat,
              clusters,
              labels_mat,
              cv::TermCriteria(CV_TERMCRIT_ITER|CV_TERMCRIT_EPS, 100, 0.01),
              1,
              cv::KMEANS_PP_CENTERS,
              centers_mat );

  // Sort labels based on commonality
  typedef std::map< unsigned, unsigned > label_map;
  const unsigned label_count = centers_mat.size().height;
  std::vector< unsigned > commonality_count( label_count, 0 );
  label_map sorted_labels;

  for( int j = 0; j < labels_mat.size().height; j++ )
  {
    commonality_count[ labels_mat.at<int>( j, 0 ) ]++;
  }
  for( unsigned j = 0; j < commonality_count.size(); j++ )
  {
    sorted_labels[ commonality_count[j] ] = j;
  }

  // Copy back to output format
  for( label_map::iterator i = sorted_labels.begin(); i != sorted_labels.end(); ++i )
  {
    std::vector<PixType> new_center( nplanes, 0 );
    for( unsigned p = 0; p < nplanes; p++ )
    {
      new_center[p] = static_cast<PixType>( centers_mat.at<float>( i->second, p ) );
    }
    centers.push_back( new_center );
  }
}

#elif USE_MUL

// Cluster 'sample_points' points from image 'img' into 'clusters' clusters.
template< typename PixType >
void mul_cluster( const vil_image_view<PixType>& image,
                  const unsigned clusters,
                  const unsigned desired_sample_points,
                  std::vector< std::vector<PixType> >& centers )
{
  centers.clear();

  const unsigned nplanes = image.nplanes();
  const unsigned image_pixels = image.ni() * image.nj();
  const unsigned pts_to_sample = std::min( desired_sample_points, image_pixels );
  const unsigned sample_step = image_pixels / pts_to_sample;
  const double type_max = static_cast<double>(std::numeric_limits<PixType>::max());

  const std::ptrdiff_t pstep = image.planestep();

  std::vector<vnl_vector<double> > data( pts_to_sample, vnl_vector<double>(nplanes, 0.0));

  // Sample image points at different locations within the image
  unsigned pos = 0;

  for( unsigned pt = 0; pt < pts_to_sample; pt++, pos += sample_step )
  {
    unsigned si = pos % image.ni();
    unsigned sj = ( pos / image.ni() ) % image.nj();

    const PixType *ptr = &image(si,sj,0);
    data[ pt ]( 0 ) = static_cast<double>(*ptr);

    for( unsigned plane = 1; plane < nplanes; plane++ )
    {
      ptr += pstep;
      data[ pt ]( plane ) = static_cast<double>(*ptr);
    }
  }

  // Initialize starting centers evenly across the range of the input type
  std::vector<vnl_vector<double> > kmeans_centers;
  vnl_vector<double> new_entry( nplanes, 0.0 );
  for( unsigned i = 0; i < clusters; i++ )
  {
    new_entry.fill( type_max * i / ( clusters - 1 ) );
    kmeans_centers.push_back( new_entry );
  }

  // Run kmeans
  mbl_data_array_wrapper<vnl_vector<double> > data_array(data);

  // Copy back to output format
  for( unsigned j = 0; j < kmeans_centers.size(); j++ )
  {
    std::vector<PixType> new_center( nplanes, 0 );
    for( unsigned p = 0; p < kmeans_centers[ j ].size(); p++ )
    {
      new_center[p] = static_cast<PixType>( kmeans_centers[ j ]( p ) );
    }
    centers.push_back( new_center );
  }
}

#endif

template< typename PixType, typename LabelType >
bool segment_image_kmeans( const vil_image_view< PixType >& src,
                           vil_image_view< LabelType >& labels,
                           const unsigned clusters,
                           const unsigned sample_points )
{
  // Validate inputs
  LOG_ASSERT( clusters <= static_cast<unsigned>( std::numeric_limits<LabelType>::max() ),
              "Cluster count exceeds LabelType's descriptive power." );

  std::vector< std::vector< PixType > > centers;

#ifdef USE_OPENCV
  ocv_cluster( src, clusters, sample_points, centers );
#elif USE_MUL
  mul_cluster( src, clusters, sample_points, centers );
#else
  // Kill unused parameter warnings
  (void) src;
  (void) clusters;
  (void) labels;
  (void) sample_points;
  (void) centers;

  // Output error statement
  LOG_ERROR( "Kmeans segmenter currently requires a build with OpenCV or MUL enabled" );
  return false;
#endif

  label_image_from_centers( src, centers, labels );
  return true;
}

}
