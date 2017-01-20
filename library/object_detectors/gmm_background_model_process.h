/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef gmm_background_model_process_h_
#define gmm_background_model_process_h_

#include <object_detectors/fg_image_process.h>
#include <process_framework/pipeline_aid.h>
#include <vgl/algo/vgl_h_matrix_2d.h>

#include <boost/scoped_ptr.hpp>

namespace vidtk
{


template<class PixType>
class gmm_background_model_process
  : public fg_image_process<PixType>
{
public:
  typedef gmm_background_model_process self_type;

  gmm_background_model_process( std::string const& name );

  virtual ~gmm_background_model_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool reset();

  virtual bool step();

  /// Specify the next image.
  ///
  /// This should be called prior to each call to step().  The image
  /// object must persist until step() is complete.
  void set_source_image( vil_image_view<PixType> const& src );

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

  /// Read in the image to world homography for the current image
  void set_homography( vgl_h_matrix_2d<double> homog );

  VIDTK_INPUT_PORT( set_homography, vgl_h_matrix_2d<double> );

  /// Return the foreground detection image
  virtual vil_image_view<bool> fg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  /// A representative image of the background.
  virtual vil_image_view<PixType> bg_image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

protected:
  void warp_array();

private:
  /// private implementation class
  class priv;
  boost::scoped_ptr<priv> d;
};


} // end namespace vidtk


#endif // gmm_background_model_process_h_
