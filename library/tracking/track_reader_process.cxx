/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include "mit_layer_annotation_reader.h"
#include "viper_reader.h"
#include "kw18_reader.h"
#include "vsl_reader.h"
#include "track_reader_process.h"
#include "glob_to_step_reader.h"

namespace vidtk
{
  track_reader_process
    ::track_reader_process(vcl_string const& name)
    : process(name, "track_reader_process"),
      format_( -1 ),
      disabled_(false),
      batch_mode_(false)
  {
    config_.add("disabled","false");
    config_.add("format","");
    config_.add("filename","");
    config_.add("ignore_occlusions","true");
    config_.add("ignore_partial_occlusions","true");
    config_.add("frequency",".5"); /*MIT Reader Only*/
    config_.add("path_to_images",""); /*Optional KW18 Only*/
    reader_ = 0;
  }

  track_reader_process
    ::~track_reader_process()
  {
    if(reader_)
    {
      delete reader_;
    }
  }

  config_block
    track_reader_process
    ::params() const
  {
    return config_;
  }

  bool
    track_reader_process
    ::set_params(config_block const&blk)
  {
    try
    {
      blk.get( "disabled", disabled_ );
      vcl_string fmt_str;
      blk.get( "format", fmt_str );
      blk.get( "filename", filename_ );
      blk.get( "ignore_occlusions", ignore_occlusions_ );
      blk.get( "ignore_partial_occlusions", ignore_partial_occlusions_ );
      blk.get( "frequency" , frequency_ );
      blk.get( "path_to_images", path_to_images_);

      // ensure that a filename has been set
      if ("" == filename_){
        log_error( name() << ": filename not set\n" );
        return false;
      }

      format_ = unsigned(-1);
      if( fmt_str == "kw18" )
      {
        format_ = 0;
      }
      else if( fmt_str == "mit")
      {
        format_ = 1;
      }
      else if( fmt_str == "viper")
      {
        format_ = 2;
      }
      else if( fmt_str == "vsl" )
      {
        format_ = 3;
      }
      else
      {
        vcl_string file_ext = filename_.substr(filename_.rfind(".")+1);
        if(file_ext == "kw18" || file_ext == "trk" || file_ext == "dat")
        {
          format_ = 0;
        }
        else if(file_ext == "xml")
        {
          format_ = 1;
        }
        else if(file_ext == "xgtf")
        {
          format_ = 2;
        }
        else if(file_ext == "vsl")
        {
          format_ = 3;
        }
        else
        {
          log_error( name() << ": unknown format " << fmt_str << "\n" );
          log_error( name() << ": unknown extension " << file_ext << "\n");
        }
      }

      // Validate the format
      if( format_ > 3 )
      {
        throw unchecked_return_value("");
      }
    }
    catch( unchecked_return_value& )
    {
      // Reset to old values
      this->set_params( config_ );
      return false;
    }

    config_.update( blk );
    return true;
  }

  bool
    track_reader_process
    ::initialize()
  {
    if (disabled_)
    {
      return true;
    }
    if(reader_)
    {
      delete reader_;
    }
    switch (format_)
    {
    case 0:
    {
      kw18_reader *r = new kw18_reader();
      r->set_path_to_images(path_to_images_);
      if( batch_mode_ )
      {
        // read all tracks in a batch mode (one step)
        reader_ = r;
      }
      else
      {
        // read tracks in multiple steps
        reader_ = new glob_to_step_reader(r,true);
      }
      break;
    }
    case 1:
    {
      if( batch_mode_ )
        reader_ = new mit_layer_annotation_reader();
      else
      {
        mit_layer_annotation_reader * r = new mit_layer_annotation_reader();
        r->set_frequency_of_frames(frequency_);
        reader_ = new glob_to_step_reader(r,true);
      }
      break;
    }
    case 2:
    {
      if( batch_mode_ )
        reader_ = new viper_reader();
      else
      {
        viper_reader * r = new viper_reader();
        reader_ = new glob_to_step_reader(r,true);
      }
      break;
    }
    case 3:
    {
      reader_ = new vsl_reader();
      break;
    }

    default:
      log_error( name() << ": unknown format number " << format_ << "\n");
      return false;
    }
    reader_->set_filename(filename_.c_str());
    reader_->set_ignore_occlusions(ignore_occlusions_);
    reader_->set_ignore_partial_occlusions(ignore_partial_occlusions_);
    return true;
  }

  bool
    track_reader_process
    ::step()
  {
    if(disabled_)
      return true;

    if( batch_mode_ && format_ == 3 )
    {
      // batch mode for vsl reader
      bool result;
      bool flag = false;
      do
      {
        vcl_vector<track_sptr> step_tracks;
        result = reader_->read( step_tracks );
        if( result )
          flag = true;
        tracks_.insert( tracks_.end(), step_tracks.begin(), step_tracks.end() );
      } while(result);
      return flag;

    }

    return reader_->read(tracks_);
  }

  vcl_vector<track_sptr> const& track_reader_process::
    tracks() const
  {
    return tracks_;
  }

  bool track_reader_process::
    is_disabled() const
  {
    return disabled_;
  }

  bool track_reader_process::
    is_batch_mode() const
  {
    return batch_mode_;
  }

  void track_reader_process::
    set_batch_mode( bool batch_mode )
  {
    batch_mode_ = batch_mode;
  }

}
