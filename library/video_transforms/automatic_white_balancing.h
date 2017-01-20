/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_simple_auto_white_balance_h_
#define vidtk_simple_auto_white_balance_h_

#include <vil/vil_image_view.h>

#include <vector>

namespace vidtk
{

// Definition of the type (default) definitions of the value of white
template <class T> struct default_white_point
{
  static T value(){ return 1; }
};
template <> struct default_white_point<vxl_byte>
{
  static vxl_byte value(){ return 255; }
};
template <> struct default_white_point<vxl_uint_16>
{
  static vxl_uint_16 value(){ return 65535; }
};
template <> struct default_white_point<unsigned int>
{
  static unsigned int value(){ return 65535; }
};
template <> struct default_white_point<int>
{
  static int value(){ return 65535; };
};


// All of the possible settings used in the below class
struct auto_white_balancer_settings
{
  auto_white_balancer_settings() :
                   white_point_value_( 0 ),
                   white_traverse_factor( 0.95 ),
                   white_ref_weight( 1.0 ),
                   black_traverse_factor( 0.75 ),
                   black_ref_weight( 1.0 ),
                   exp_averaging_factor( 0.25 ),
                   correction_matrix_res( 8 ),
                   pixels_to_sample( 10000 ) {}

  // Approximate definition of pure white, a value of 0 indcates to use the type default
  double white_point_value_;

  // Shift the whitest thing in the image this percentage towards pure white
  double white_traverse_factor;
  double white_ref_weight;

  // Shift the blackest thing in the image this percentage towards pure black
  double black_traverse_factor;
  double black_ref_weight;

  // Exponential averaging coefficient for averaging past correction factors
  double exp_averaging_factor;

  // Resolution of matrix (per channel) when storing correctional shifts
  unsigned correction_matrix_res;

  // When estimating a correction matrix for some image, the desired pixel count
  unsigned pixels_to_sample;
};


// A simple helper struct for storing reference points (some color and a
// vector in the direction of what that color should ideally map to)
template <typename PixType, unsigned int Channels = 3>
class awb_reference_point
{
public:

  awb_reference_point( const PixType* pt, const PixType* ref, double weight )
  {
    for( unsigned int i = 0; i < Channels; i++ )
    {
      loc_[i] = pt[i];
      vec_[i] = static_cast<double>(ref[i])-static_cast<double>(loc_[i]);
    }
    weight_ = weight;
  }

  void scale_vector( const double& factor )
  {
    for( unsigned int i = 0; i < Channels; i++ )
    {
      vec_[i] = factor * vec_[i];
    }
  }

  PixType loc_[Channels];
  double vec_[Channels];
  double weight_;
};


// Given multiple reference points, compute a correction matrix by interpolating
// an estimated shift for each bin in the color space
template <typename PixType>
void compute_correction_matrix( std::vector< awb_reference_point< PixType > >& ref,
                                const unsigned& resolution_per_chan,
                                std::vector< double >& output,
                                const double& white_point );


// A process-like class which can compute some color space warping and
// apply it to some group of images. A correction matrix covering the entire
// RGB color space is created by identifying potential reference points and
// what colors they should ideally map to, and then by interpolating an approx
// shift (a 3D vector) for each bin in this correction matrix sampling the color
// space.
template <typename PixType>
class auto_white_balancer
{

public:

  typedef vil_image_view< PixType > input_type;
  typedef std::vector< double > matrix_type;

  // Constructor/Destructor
  auto_white_balancer();
  ~auto_white_balancer();

  // Reset recorded history/averages (should be called near shot breaks)
  void reset();

  // Set new options for the filter
  void configure( auto_white_balancer_settings& options );

  // Creates a correction matrix from the image, and applies it to it
  void apply( input_type& image );

  // Creates a correction matrix from some reference image, and applies
  // it to the first input image
  void apply( input_type& image, const input_type& reference );

private:

  // Stored variables
  auto_white_balancer_settings options_;
  matrix_type correction_matrix_;
  matrix_type temp_matrix_;
  input_type downsized_image_;
  input_type smoothed_image_;
  bool is_first_matrix_;

  // Apply a correction matrix to some image using the internal settings
  void apply_correction_matrix( input_type& image, matrix_type& matrix );

  // Helper function which calculates the neareset point (L1) distance to some
  // reference point. L1 distance is used for efficiency and because we may often
  // have a noticable skew of just a single principle color.
  void calculate_nearest_point_l1( const input_type& image,
                                   const PixType* reference,
                                   PixType* nearest,
                                   unsigned sample_rate = 1 );
};

}

#endif // vidtk_auto_white_balancing_h_
