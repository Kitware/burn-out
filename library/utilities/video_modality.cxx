/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_modality.h"

namespace vidtk
{

std::string video_modality_to_string( video_modality vm )
{
  std::string str;
  switch( vm )
  {
    case VIDTK_COMMON:  str = "Common";      break;
    case VIDTK_EO_H:    str = "EO_HFOV";     break;
    case VIDTK_EO_M:    str = "EO_MFOV";     break;
    case VIDTK_EO_N:    str = "EO_NFOV";     break;
    case VIDTK_IR_H:    str = "IR_HFOV";     break;
    case VIDTK_IR_M:    str = "IR_MFOV";     break;
    case VIDTK_IR_N:    str = "IR_NFOV";     break;
    case VIDTK_EO_FB:   str = "EO_FALLBACK"; break;
    case VIDTK_IR_FB:   str = "IR_FALLBACK"; break;
    case VIDTK_EO:      str = "EO_?";        break;
    case VIDTK_IR:      str = "IR_?";        break;
    default:            str = "INVALID_TYPE";
  }

  return str;
}

bool is_video_modality_ir( video_modality vm )
{

  switch( vm )
  {
    case VIDTK_IR_H:
    case VIDTK_IR_M:
    case VIDTK_IR_N:
    case VIDTK_IR_FB:
    case VIDTK_IR:
      return true;

    default:
      return false;
  }

  return false;
}

bool is_video_modality_eo( video_modality vm )
{

  switch( vm )
  {
    case VIDTK_COMMON:
    case VIDTK_EO_H:
    case VIDTK_EO_M:
    case VIDTK_EO_N:
    case VIDTK_EO_FB:
    case VIDTK_EO:
      return true;

    default:
      return false;
  }

  return false;
}

bool is_video_modality_invalid( video_modality vm )
{
  return ( vm == VIDTK_INVALID || vm >= VIDTK_VIDEO_MODALITY_END );
}

}; //namespace vidtk
