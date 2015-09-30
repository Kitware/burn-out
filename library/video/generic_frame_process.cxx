/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video/generic_frame_process.h>

#include <video/vidl_ffmpeg_frame_process.h>
#include <video/image_list_frame_process.h>
#include <video/image_list_frame_metadata_process.h>

namespace vidtk
{


template<>
void
generic_frame_process<bool>
::populate_possible_implementations()
{
  impls_.push_back( vcl_make_pair( "image_list",
                                   new image_list_frame_process<bool>(
                                     this->name()+"/imagelist" ) ) );
  impls_.push_back( vcl_make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<bool>(
                                     this->name()+"/image_metadata_list" ) ) );

}


template<>
void
generic_frame_process<vxl_byte>
::populate_possible_implementations()
{
  impls_.push_back( vcl_make_pair( "vidl_ffmpeg",
                                   new vidl_ffmpeg_frame_process(
                                     this->name()+"/ffmpeg" ) ) );

  impls_.push_back( vcl_make_pair( "image_list",
                                   new image_list_frame_process<vxl_byte>(
                                     this->name()+"/imagelist" ) ) );

  impls_.push_back( vcl_make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<vxl_byte>(
                                     this->name()+"/image_metadata_list" ) ) );

}


template<>
void
generic_frame_process<vxl_uint_16>
::populate_possible_implementations()
{
  impls_.push_back( vcl_make_pair( "image_list",
                                   new image_list_frame_process<vxl_uint_16>(
                                     this->name()+"/imagelist" ) ) );

  impls_.push_back( vcl_make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<vxl_uint_16>(
                                     this->name()+"/image_metadata_list" ) ) );

}


} // end namespace vidtk
