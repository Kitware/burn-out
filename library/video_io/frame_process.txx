/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#ifndef VIDTK_FRAME_PROCESS_TXX_
#define VIDTK_FRAME_PROCESS_TXX_

#include "frame_process.h"

#include <vul/vul_reg_exp.h>



#include <logger/logger.h>
#undef VIDTK_DEFAULT_LOGGER
#define VIDTK_DEFAULT_LOGGER __vidtk_logger_auto_frame_process_txx__
VIDTK_LOGGER("frame_process_txx");


namespace vidtk
{

template <class PixType>
bool
frame_process<PixType>
::set_roi(std::string & roi)
{
  //LOG_INFO( "ROI: " << roi );
  vul_reg_exp roi_re( "([0-9]+).([0-9]+).([0-9]+).([0-9]+)" );
  if ( ! roi_re.find( roi ))
  {
    LOG_ERROR( "ROI string '" << roi << "': regexp failure" );
    return false;
  }
  std::istringstream iss( roi_re.match(1)+" "+roi_re.match(2)+" "+roi_re.match(3)+" "+roi_re.match(4) );
  unsigned int rw,rh,rx,ry;
  if ( ! (iss >> rw >> rh >> rx >> ry) )
  {
    LOG_ERROR( "ROI string '" << roi << ": couldn't parse 4 unsigned ints from '" << iss.str() << "'" );
    return false;
  }
  //Check the sizes
  if( !set_roi(rx,ry,rw,rh) )
  {
    return false;
  }
  has_roi_ = true;
  return true;
}

template< class PixType >
bool
frame_process<PixType>
::set_roi( unsigned int roi_x, unsigned int roi_y,
           unsigned int roi_width, unsigned int roi_height )
{
  roi_x_ = roi_x;
  roi_y_ = roi_y;
  roi_width_ = roi_width;
  roi_height_ = roi_height;
  has_roi_ = true;
  //TODO check sizes;
//   if(roi_x_ > this->ni() || roi_y_ > this->nj() ||
//      roi_x_ + roi_width_ > this->ni() || roi_y_ + roi_height_ > this->nj() )
//   {
//     LOG_ERROR( "ROI string '" << roi << ": is not completely inside image '" << ni_ << " " << nj_ << "'" );
//     return false;
//   }
  return true;
}

template< class PixType >
void
frame_process<PixType>
::turn_off_roi()
{
  has_roi_ = false;
}

template< class PixType >
vidtk::video_metadata
frame_process<PixType>
::metadata() const
{
  return metadata_;
}

}

#endif
