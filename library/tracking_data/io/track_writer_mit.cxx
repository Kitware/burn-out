/*ckwg +5
 * Copyright 2010-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_writer_mit.h"

#include <tracking_data/tracking_keys.h>

#include <logger/logger.h>

#include <utilities/geo_lat_lon.h>

#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>
#include <vul/vul_string.h>

namespace vidtk
{

VIDTK_LOGGER ("mit_writer");

track_writer_mit
::track_writer_mit( std::string path_to_images,
                    int x_offset, int y_offset )
: fstr_(NULL), path_to_images_(path_to_images),
  x_offset_(x_offset), y_offset_(y_offset), num_imgs_(0)
{
}

track_writer_mit
::~track_writer_mit()
{
  this->close();
  delete fstr_;
}

void
track_writer_mit
::set_options(track_writer_options const& options)
{
  path_to_images_ = options.path_to_images_;
  x_offset_ = options.x_offset_;
  y_offset_ = options.y_offset_;
}

bool
track_writer_mit
::write( vidtk::track const & track )
{
  if(!is_good())
  {
    return false;
  }
  std::vector< vidtk::track_state_sptr > const& hist = track.history();
  unsigned M = hist.size();
  (*fstr_) << "\
          <object>\n\
            <name>"<<track.id()<<"</name>\n\
            <notes></notes>\n\
            <deleted>0</deleted>\n\
            <verified>0</verified>\n\
            <date>20-Aug-2008 10:27:25</date>\n\
            <id>"<<track.id()<<"</id>\n\
            <username>anonymous</username>\n\
            <numFrames>"<<num_imgs_<<"</numFrames>\n\
            <startFrame>"<<hist[0]->time_.frame_number()<<"</startFrame>\n\
            <endFrame>"<<hist[M-1]->time_.frame_number()<<"</endFrame>\n\
            <createdFrame>"<<hist[0]->time_.frame_number()<<"</createdFrame>\n\
            <labels>\n";

  for( std::vector< vidtk::track_state_sptr >::const_iterator iter = hist.begin(); iter != hist.end(); ++iter )
  {
    (*fstr_) << "\
                <frame>\n\
                    <index>"<<(*iter)->time_.frame_number()<<"</index>\n\
                    <depth>200</depth>\n\
                    <globalLabel>1</globalLabel>\n\
                    <localLabel>1</localLabel>\n\
                    <tracked>1</tracked>\n\
                    <depthLabel>1</depthLabel>\n\
                    <isPolygon>1</isPolygon>\n\
                    <polygon>\n\
                        <pt>\n\
                              <x>"<<(*iter)->amhi_bbox_.min_x()+x_offset_<<"</x>\n\
                              <y>"<<(*iter)->amhi_bbox_.min_y()+y_offset_<<"</y>\n\
                              <labeled>1</labeled>\n\
                        </pt>\n\
                        <pt>\n\
                              <x>"<<(*iter)->amhi_bbox_.min_x()+x_offset_<<"</x>\n\
                              <y>"<<(*iter)->amhi_bbox_.max_y()+y_offset_<<"</y>\n\
                              <labeled>1</labeled>\n\
                        </pt>\n\
                        <pt>\n\
                              <x>"<<(*iter)->amhi_bbox_.max_x()+x_offset_<<"</x>\n\
                              <y>"<<(*iter)->amhi_bbox_.max_y()+y_offset_<<"</y>\n\
                              <labeled>1</labeled>\n\
                        </pt>\n\
                        <pt>\n\
                              <x>"<<(*iter)->amhi_bbox_.max_x()+x_offset_<<"</x>\n\
                              <y>"<<(*iter)->amhi_bbox_.min_y()+y_offset_<<"</y>\n\
                              <labeled>1</labeled>\n\
                        </pt>\n\
                      </polygon>\n\
                </frame>\n";
  }
  (*fstr_) << "\
            </labels>\n\
        </object>\n";
  fstr_->flush();
  return this->is_good();
}

bool
track_writer_mit
::open( std::string const& fname )
{
  this->close();
  delete fstr_;
  fstr_ = new std::ofstream(fname.c_str());

  std::string file_names = "";

  if(vul_file::exists( path_to_images_) )
  {
    std::string glob_str = path_to_images_;
    char lastc = path_to_images_.c_str()[path_to_images_.length()-1];
    if(lastc != '\\' && lastc != '/')
    {
      glob_str += "/";
    }
    glob_str += "*";
    for(vul_file_iterator iter = glob_str ; iter; ++iter)
    {
      std::string ext = vul_file::extension(iter.filename());
      ext = vul_string_downcase(ext);
      if( ext == ".jpg" || ext == ".png" || ext == ".jpeg")
      {
        file_names += "      <fileName>";
        file_names += iter.filename();
        file_names += "</fileName>\n";
        num_imgs_++;
      }
    }
  }
  else
  {
    LOG_ERROR("Path to images must be set for mit files");
    return false;
  }

  (*fstr_) << "\
    <annotation>\n\
      <version>1.00</version>\n\
      <videoType>frames</videoType>\n\
      <folder>" << path_to_images_ << "</folder>\n\
      <NumFrames>" << num_imgs_ << "</NumFrames>\n\
      <currentFrame>0</currentFrame>\n\
      <fileList>\n" <<
        file_names << "\
      </fileList>\n\
      <source>\n\
        <sourceImage>Videos from source provided to tracker</sourceImage>\n\
        <sourceAnnotation>Files created by Kitware's VIDTK MIT writer</sourceAnnotation>\n\
      </source>\n";

  if(!this->is_good())
  {
    LOG_ERROR("Could not open " << fname);
    return false;
  }
  return true;
}

bool
track_writer_mit
::is_open() const
{
  return this->is_good();
}

void
track_writer_mit
::close()
{
  if(fstr_)
  {
    if(is_open())
    {
      (*fstr_) << "</annotation>\n";
    }
    num_imgs_ = 0;
    fstr_->close();
  }
}

bool
track_writer_mit
::is_good() const
{
  return fstr_ && fstr_->good();
}

}//namespace vidtk
