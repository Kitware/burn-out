/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "image_chip_writer_process.h"

#include <utilities/log.h>
#include <utilities/unchecked_return_value.h>
#include <utilities/vsl/timestamp_io.h>
#include <vsl/vsl_binary_io.h>
#include <vsl/vsl_vector_io.h>
#include <vsl/vsl_vector_io.txx>
#include <vbl/io/vbl_io_smart_ptr.h>
#include <vbl/io/vbl_io_smart_ptr.txx>
#include <tracking/vsl/track_io.h>
#include <tracking/vsl/image_object_io.h>
#include <vul/vul_file.h>
#include <vul/vul_file_iterator.h>
#include <vil/vil_save.h>
#include <vil/vil_convert.h>

#include <vcl_cstdio.h>
#include <vbl/vbl_smart_ptr.txx>
#include <vpl/vpl.h>

namespace vidtk
{
image_chip_writer_process
::image_chip_writer_process( vcl_string const& name )
  : process( name, "image_chip_writer_process" ),
    image_chip_disabled_( true ),
    alpha_chip_disabled_( true ),
    allow_overwrite_( true ),
    src_trks_( NULL )
{
  config_.add_parameter( "image_chip_disabled", "true", "Disables the image chip writing" );
  config_.add_parameter( "alpha_chip_disabled", "true", "Disables the alpha mask chip writing" );
  config_.add_parameter( "chip_location", "", "The location of the mover chips" ); //in printf form
  config_.add_parameter( "image_chip_filename_format", "", "The image chip output file format" ); //in printf form
  config_.add_parameter( "alpha_chip_filename_format", "", "The alpha mask chip output file format" ); //in printf form
  config_.add_parameter( "overwrite_existing", "false", 
                         "Whether or not a file can be overwriten" );
  config_.add_parameter( "frequency", "0",
                         "The number of frames per second" );
}


image_chip_writer_process
::~image_chip_writer_process()
{
}


config_block
image_chip_writer_process
::params() const
{
  return config_;
}

bool
image_chip_writer_process
::set_params( config_block const& blk )
{
  try
  {
    blk.get( "image_chip_disabled", image_chip_disabled_ );
    blk.get( "alpha_chip_disabled", alpha_chip_disabled_);
    blk.get( "chip_location", chip_location_);
    blk.get( "image_chip_filename_format", image_chip_filename_format_);
    blk.get( "alpha_chip_filename_format", alpha_chip_filename_format_);
    blk.get( "overwrite_existing", allow_overwrite_ );
	  blk.get( "frequency", frequency_ ); 
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
image_chip_writer_process
::initialize()
{
  return true;
}


bool
image_chip_writer_process
::step()
{
  if( src_trks_ == NULL )
  {
    vcl_cerr<< name() << ": source tracks not specified."<< vcl_endl;
    return false;
  }
  
  for(unsigned int i = 0; i < src_trks_->size(); ++i)
  {
    track_state_sptr trkStPtr;
    vcl_vector< track_state_sptr > const & hist = (*src_trks_)[i]->history();
    for(unsigned int j = 0; j < hist.size(); j++)
    {
      trkStPtr = hist[j];
      vcl_vector<vidtk::image_object_sptr> objs;
      if( trkStPtr->data_.get(vidtk::tracking_keys::img_objs, objs) )
      {
        for(unsigned int k = 0; k < objs.size(); ++k)
        {
          unsigned int frameNo, rowNo, colNo, objNo = 0;
          char objDir[500];
          if(!alpha_chip_disabled_ || !image_chip_disabled_)
          {
            frameNo = hist[j]->time_.frame_number();
            rowNo = objs[k]->mask_i0_;
            colNo = objs[k]->mask_j0_;
            objNo = (*src_trks_)[i]->id();
            
            sprintf(objDir, chip_location_.c_str(), objNo);
            vpl_mkdir(objDir, 0777);
          }
          if(!alpha_chip_disabled_)
          {
            char alphaDir[500];
            strcpy(alphaDir, objDir);
            strcat(alphaDir, "alpha_chips/");
            vpl_mkdir(alphaDir, 0777);
            
            mChip_view aMask = objs[k]->mask_;
            iChip_view aMaskPrint;
            vil_convert_stretch_range(aMask, aMaskPrint);
            
            char chipFilePath[1000];
            strcpy(chipFilePath, alphaDir);
            char chipFilename[200];
            sprintf(chipFilename, alpha_chip_filename_format_.c_str(), frameNo, objNo, rowNo, colNo);
            strcat(chipFilePath, chipFilename);
            vcl_string maskFilename = chipFilePath;
            if(aMask.ni() > 0 && aMask.nj() > 0)
            {
              write_alpha_chip(maskFilename, aMaskPrint);
            }
          }
          if(!image_chip_disabled_)
          {
            char imDir[500];
            strcpy(imDir, objDir);
            strcat(imDir, "image_chips/");
            vpl_mkdir(imDir, 0777);
            
            iChip_view imChip;
            objs[k]->data_.get(vidtk::tracking_keys::pixel_data, imChip);
            
            
            char chipFilePath[1000];
            strcpy(chipFilePath, imDir); 
            char chipFilename[200];
            sprintf(chipFilename, image_chip_filename_format_.c_str(), frameNo, objNo, rowNo, colNo);
            strcat(chipFilePath, chipFilename);
            vcl_string temp = chipFilePath;
            if(imChip.ni() > 0 && imChip.nj() > 0)
            {
              write_image_chip(chipFilePath, imChip);
            }
          }
        }
      }
    }
  }
  src_trks_ = NULL;
  return true;
}

void
image_chip_writer_process
::set_tracks( vcl_vector< track_sptr > const& trks )
{
  src_trks_ = &trks;
}

bool
image_chip_writer_process
::is_image_chip_disabled() const
{
  return image_chip_disabled_;
}

bool
image_chip_writer_process
::is_alpha_chip_disabled() const
{
  return alpha_chip_disabled_;
}

/// Returns true if the file filename exists and we are not
/// allowed to overwrite it (governed by allow_overwrite_).
bool
image_chip_writer_process
::fail_if_exists( vcl_string const& filename )
{
  return ( ! allow_overwrite_ ) && vul_file::exists( filename );
}

void
image_chip_writer_process
::write_image_chip(vcl_string const& filename, iChip_view const& imageChip)
{
  if(fail_if_exists(filename))
  {
    log_error( name() << "File: "
    << filename << " already exists. To overwrite set allow_overwrite flag\n" );
    return;
  }
  vil_save(imageChip, filename.c_str());
}

void
image_chip_writer_process
::write_alpha_chip(vcl_string const& filename, iChip_view const& alphaChip)
{
   if(fail_if_exists(filename))
  {
    log_error( name() << "File: "
    << filename << " already exists. To overwrite set allow_overwrite flag\n" );
    return;
  }
  vil_save(alphaChip, filename.c_str());
}

} // end namespace vidtk
