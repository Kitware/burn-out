/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_rescale_image_process_h_
#define vidtk_rescale_image_process_h_

#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

template <class PixType>
class vil_image_view;

namespace vidtk
{


template <class PixType>
class rescale_image_process
  : public process
{
public:
  typedef rescale_image_process self_type;

  rescale_image_process( std::string const& name );
  virtual ~rescale_image_process();

  // -- process interface
  virtual config_block params() const;
  virtual bool set_params( config_block const& );
  virtual bool initialize();
  virtual bool reset();
  virtual bool step();

  void set_image( vil_image_view<PixType> const& img );
  VIDTK_INPUT_PORT( set_image, vil_image_view<PixType> const& );

  vil_image_view<PixType> image() const;
  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, image );

private:
  typedef enum
  {
    rescale_pass,
    rescale_bilinear,
    rescale_decimate
  } rescale_algorithm_t;

  config_block config_;

  rescale_algorithm_t algo_;
  double scale_factor_;
  unsigned decimation_;

  vil_image_view< PixType > in_img_;
  vil_image_view< PixType > out_img_;
};


} // end namespace vidtk


#endif // vidtk_rescale_image_process_h_
