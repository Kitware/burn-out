/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef vidtk_extract_matrix_process_h_
#define vidtk_extract_matrix_process_h_

#include <vgl/algo/vgl_h_matrix_2d.h>
#include <process_framework/process.h>
#include <process_framework/pipeline_aid.h>

#include <utilities/homography.h>

namespace vidtk
{


template <typename Homography>
class extract_matrix_process
  : public process
{
public:
  typedef extract_matrix_process self_type;
  typedef process_smart_pointer<self_type> pointer;

  extract_matrix_process( std::string const& name );

  ~extract_matrix_process();

  virtual config_block params() const;

  virtual bool set_params( config_block const& );

  virtual bool initialize();

  virtual bool step();

  void set_homography( Homography const & h );
  VIDTK_INPUT_PORT( set_homography, Homography const & );

  /// Output port(s)

  vgl_h_matrix_2d<double> matrix() const;
  VIDTK_OUTPUT_PORT( vgl_h_matrix_2d<double>, matrix );

private:

  config_block config_;

  Homography homography_;
  vgl_h_matrix_2d<double> matrix_;
};


} // end namespace vidtk


#endif // vidtk_extract_matrix_process_h_
