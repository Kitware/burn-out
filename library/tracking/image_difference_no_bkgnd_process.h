/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_image_difference_process_h_
#define vidtk_image_difference_process_h_

#include <vcl_vector.h>

#include <vil/vil_image_view.h>
#include <tracking/fg_image_process.h>
#include <process_framework/pipeline_aid.h>
#include <vcl_fstream.h>

namespace vidtk
{

/// Find moving object by pixelwise comparison of registered images.
///
/// This class is a simpler version of image_difference that will 
/// detect bright moving objects against a fixed background.

template < class PixType >
class image_difference_no_bkgnd_process
  : public fg_image_process<PixType>
{
public:
  typedef image_difference_no_bkgnd_process self_type;

  image_difference_no_bkgnd_process( vcl_string const& name );
  
  ~image_difference_no_bkgnd_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_source_image( vil_image_view<PixType> const& src );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// the difference image between the current source image and a 
  /// dark back ground image. 
  virtual vil_image_view<bool> const& fg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  /// \brief A representative image of the background.
  virtual vil_image_view<PixType> bg_image() const;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

protected:
  config_block config_;

  // i_8 = (i_16 - min_16) * 255 / (max_16 - min_16)
  PixType conv_16bit_2_8bit( unsigned short i_16, unsigned short min_16bit, unsigned short max_16bit ) 
  {
      return PixType( (i_16 - min_16bit) * double(std::numeric_limits<PixType>::max()) / (max_16bit - min_16bit) ); 
  };

  unsigned short conv_8bit_2_16bit( PixType i_8, unsigned short min_16bit, unsigned short max_16bit ) 
  {
    return static_cast<unsigned short>( 
                (double(i_8)/std::numeric_limits<PixType>::max())
                * (max_16bit - min_16bit) + min_16bit ); 
  };

  vil_image_view<PixType> const* src_img_;
  vil_image_view<PixType> bg_img_;

  /// current difference image.
  vil_image_view<bool> diff_image_;
  vil_image_view<bool> bg_image_;

  /// Parameters
  double diff_threshold_percentage_;
  unsigned short diff_threshold_;

  vcl_string dynamic_range_filename_;
  vcl_ifstream dynamic_range_file_; 

  double n_means_;
  unsigned init_length_;
};

} // namespace vidtk

#endif
