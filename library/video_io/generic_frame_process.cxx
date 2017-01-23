/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <video_io/generic_frame_process.h>
#include <video_io/frame_process_sp_facade.h>

#include <video_io/vidl_ffmpeg_frame_process.h>
#include <video_io/vidl_ffmpeg_metadata_frame_process.h>
#include <video_io/image_list_frame_process.h>
#include <video_io/image_list_frame_metadata_process.h>
#include <video_io/frame_metadata_super_process.h>

namespace vidtk
{


template<>
void
generic_frame_process<bool>
::populate_possible_implementations()
{
  impls_.push_back( std::make_pair( "image_list",
                                   new image_list_frame_process<bool>(
                                     this->name()+"/imagelist" ) ) );
  impls_.push_back( std::make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<bool>(
                                     this->name()+"/image_metadata_list" ) ) );
  impls_.push_back( std::make_pair( "frame_metadata",
                                   new frame_process_sp_facade
                                     < bool, frame_metadata_super_process<bool> >(
                                       this->name()+"/frame_metadata" ) ) );
}


template<>
void
generic_frame_process<vxl_byte>
::populate_possible_implementations()
{
  impls_.push_back( std::make_pair( "vidl_ffmpeg",
                                   new vidl_ffmpeg_frame_process(
                                     this->name()+"/ffmpeg" ) ) );

  impls_.push_back( std::make_pair( "vidl_ffmpeg_metadata",
                                   new vidl_ffmpeg_metadata_frame_process(
                                     this->name()+"/ffmpeg_metadata" ) ) );

  impls_.push_back( std::make_pair( "image_list",
                                   new image_list_frame_process<vxl_byte>(
                                     this->name()+"/imagelist" ) ) );

  impls_.push_back( std::make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<vxl_byte>(
                                     this->name()+"/image_metadata_list" ) ) );

  impls_.push_back( std::make_pair( "frame_metadata",
                                   new frame_process_sp_facade
                                   < vxl_byte, frame_metadata_super_process<vxl_byte> >(
                                     this->name()+"/frame_metadata" ) ) );
}


template<>
void
generic_frame_process<vxl_uint_16>
::populate_possible_implementations()
{
  impls_.push_back( std::make_pair( "image_list",
                                   new image_list_frame_process<vxl_uint_16>(
                                     this->name()+"/imagelist" ) ) );

  impls_.push_back( std::make_pair( "image_metadata_list",
                                   new image_list_frame_metadata_process<vxl_uint_16>(
                                     this->name()+"/image_metadata_list" ) ) );

  impls_.push_back( std::make_pair( "frame_metadata",
                                   new frame_process_sp_facade
                                   < vxl_uint_16, frame_metadata_super_process<vxl_uint_16> >(
                                     this->name()+"/frame_metadata" ) ) );

}


} // end namespace vidtk
