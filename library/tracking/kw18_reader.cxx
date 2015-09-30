/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "tracking/kw18_reader.h"
#include <vcl_fstream.h>
#include <tracking/tracking_keys.h>
#include <vil/vil_load.h>
#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>

using namespace vidtk;

namespace
{
void tokenize(const vcl_string& str,
                      vcl_vector<vcl_string>& tokens,
                      const vcl_string& delimiters = " \t\n\r")
{
    vcl_string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    vcl_string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (vcl_string::npos != pos || vcl_string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

typedef vcl_map<int,int> id_map;
};

bool kw18_reader::set_path_to_images(vcl_string& path)
{
  if ( path.empty() )
  {
    this->read_pixel_data = false;
    return false;
  }

  this->path_to_images_ = path;

  vcl_string glob_str = path_to_images_;
  vcl_string path_str = path_to_images_;
  char lastc = path_to_images_.c_str()[path_to_images_.length()-1];
  if(lastc != '\\' || lastc != '/')
  {
    glob_str += "/";
    path_str += "/";
  }
  glob_str += "*";
  for(vul_file_iterator iter = glob_str ; iter; ++iter)
  {
    if(vul_file::extension(iter.filename()) == ".jpg" ||
      vul_file::extension(iter.filename()) == ".png" ||
      vul_file::extension(iter.filename()) == ".jpeg")
    {
      image_names_.push_back(path_str+iter.filename());
    }
  }

  this->read_pixel_data = image_names_.size() > 0;
  return this->read_pixel_data;
}

bool
kw18_reader
::read( vcl_istream& is,
        vcl_vector< track_sptr >& trks )
{
  vcl_string line;
  id_map id_to_index;
  vcl_vector<vcl_string> col;
  vcl_vector<image_object_sptr> objs(1);
  while( !is.eof() )
  {
    vcl_getline(is,line);

    //skip blank lines
    if( line == "" )
    {
      continue;
    }
    col.clear();
    tokenize(line,col);

    //skip comments
    if(col[0][0] == '#')
    {
      continue;
    }

    if(col.size() < 18 || col.size() > 20)
    {
      vcl_cerr << "This is not a kw18 kw19 or kw20 file; found " << col.size() << " columns" << vcl_endl;
      return false;
    };

    int index;
    int id = atoi(col[0].c_str());
    id_map::iterator itr = id_to_index.find(id);
    if(itr == id_to_index.end())
    {
      vidtk::track_sptr to_push = new vidtk::track;
      to_push->set_id(id);
      trks.push_back(to_push);
      index = trks.size() - 1;
      id_to_index[id] = index;
    }
    else
    {
      index = itr->second;
    }
    vidtk::track_state_sptr new_state = new vidtk::track_state;

    new_state->time_.set_frame_number(atoi(col[2].c_str()));
    new_state->loc_[0] = atof(col[3].c_str());
    new_state->loc_[1] = atof(col[4].c_str());
    new_state->loc_[2] = 0;

    new_state->vel_[0] = atof(col[5].c_str());
    new_state->vel_[1] = atof(col[6].c_str());
    new_state->vel_[2] = 0;

    image_object_sptr obj = new image_object;
    obj->img_loc_[0] = atof(col[7].c_str());
    obj->img_loc_[1] = atof(col[8].c_str());
    obj->bbox_.set_min_x(atoi(col[9].c_str()));
    obj->bbox_.set_min_y(atoi(col[10].c_str()));
    obj->bbox_.set_max_x(atoi(col[11].c_str()));
    obj->bbox_.set_max_y(atoi(col[12].c_str()));
    obj->area_ = atof(col[13].c_str());
    obj->world_loc_[0] = atof(col[14].c_str());
    obj->world_loc_[1] = atof(col[15].c_str());
    obj->world_loc_[2] = atof(col[16].c_str());

    if(this->read_pixel_data)
    {
      //Read the image from file
      vil_image_view<vxl_byte> src_img = vil_load(this->image_names_[new_state->time_.frame_number()].c_str());

      vil_image_view<vxl_byte> bbox_pxls =
        vil_image_view<vxl_byte>(obj->bbox_.max_x() - obj->bbox_.min_x(),
                                 obj->bbox_.max_y() - obj->bbox_.min_y(),
                                 src_img.nplanes());

      obj->mask_ = vil_image_view<bool>(obj->bbox_.max_x() - obj->bbox_.min_x(),
                                        obj->bbox_.max_y() - obj->bbox_.min_y());

      for(unsigned i = 0; i < bbox_pxls.ni(); i++)
      {
        for(unsigned j = 0; j < bbox_pxls.nj(); j++)
        {
          for(unsigned p = 0; p < bbox_pxls.nplanes(); p++)
          {
            bbox_pxls(i,j,p) = src_img(i+obj->bbox_.min_x(), j+obj->bbox_.min_y(),p);
          }
          obj->mask_(i,j) = true;
        }
      }
      obj->data_.set(tracking_keys::pixel_data,bbox_pxls);
      obj->data_.set(tracking_keys::pixel_data_buffer, (unsigned int)0);
    }

    objs[0] = obj;
    new_state->data_.set( tracking_keys::img_objs, objs);

    new_state->amhi_bbox_.set_min_x(atoi(col[9].c_str()));
    new_state->amhi_bbox_.set_min_y(atoi(col[10].c_str()));
    new_state->amhi_bbox_.set_max_x(atoi(col[11].c_str()));
    new_state->amhi_bbox_.set_max_y(atoi(col[12].c_str()));

    //time_ is a timestamp which keeps time in microseconds
    //the kw18 files so far have time written in seconds
    //so, convert from seconds to microseconds
    new_state->time_.set_time( atof(col[17].c_str()) * 1e6 );

    trks[index]->add_state(new_state);

  } // ...while !eof
  return true;
}


bool kw18_reader::read(vcl_vector< track_sptr >& trks)
{
  trks.clear();
  bool close_stream_on_exit = false;
  vcl_ifstream kw18_file;
  if ( track_stream_ == 0 )  // assume we're in filename mode
  {
    if(been_read_)
    {
      return false;
    }
    kw18_file.open( filename_.c_str() );
    if ( ! kw18_file.is_open() )
    {
      vcl_cerr << "Error reading: '" << filename_ << "'" << vcl_endl;
      return false;
    }
    track_stream_ = &kw18_file;
    close_stream_on_exit = true;
  }

  bool rc = this->read( *track_stream_, trks );

  if (close_stream_on_exit)
  {
    kw18_file.close();
    been_read_ = true;
    track_stream_ = 0;
  }

  return rc;
}
