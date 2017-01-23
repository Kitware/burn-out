/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_fg_image_process_h_
#define vidtk_fg_image_process_h_

#include <process_framework/process.h>
#include <vil/vil_image_view.h>
#include <pipeline_framework/pipeline.h>
#include <pipeline_framework/pipeline_node.h>

namespace vidtk
{

template<class PixType>
class fg_image_process
  : public process
{
public:
  fg_image_process( std::string const& _name, std::string const& _class_name )
    : process( _name, _class_name )
  {
  }

  typedef fg_image_process self_type;

  virtual config_block params() const = 0;

  virtual bool set_params( config_block const& ) = 0;

  virtual bool initialize() = 0;

  virtual bool step() = 0;

  virtual vil_image_view<bool> fg_image() const = 0;
  VIDTK_OUTPUT_PORT( vil_image_view<bool>, fg_image );

  virtual vil_image_view<PixType> bg_image() const = 0;

  VIDTK_OUTPUT_PORT( vil_image_view<PixType>, bg_image );

  virtual void set_source_image( vil_image_view<PixType> const& ) = 0;

  VIDTK_INPUT_PORT( set_source_image, vil_image_view<PixType> const& );

};


} // end namespace vidtk


#endif // vidtk_fg_image_process_h_
