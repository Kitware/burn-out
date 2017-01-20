/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "extract_matrix_process.h"

#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_extract_matrix_process_cxx__
VIDTK_LOGGER("extract_matrix_process_cxx");



namespace vidtk
{


template <typename Homography>
extract_matrix_process<Homography>
::extract_matrix_process( std::string const& _name )
  : process( _name, "extract_matrix_process" )
{
}


template <typename Homography>
extract_matrix_process<Homography>
::~extract_matrix_process()
{
}


template <typename Homography>
config_block
extract_matrix_process<Homography>
::params() const
{
  return config_;
}


template <typename Homography>
bool
extract_matrix_process<Homography>
::set_params( config_block const& blk )
{
  config_.update( blk );
  return true;
}


template <typename Homography>
bool
extract_matrix_process<Homography>
::initialize()
{
  matrix_.set_identity();

  return true;
}


template <typename Homography>
bool
extract_matrix_process<Homography>
::step()
{
  matrix_ = homography_.get_transform();

  return true;
}

/// Input port(s)

template <typename Homography>
void
extract_matrix_process<Homography>
::set_homography( Homography const & h )
{
  homography_ = h;
}

/// Output port(s)

template <typename Homography>
vgl_h_matrix_2d<double>
extract_matrix_process<Homography>
::matrix() const
{
  return matrix_;
}

} // end namespace vidtk
