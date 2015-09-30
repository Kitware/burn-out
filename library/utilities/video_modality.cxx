/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "video_modality.h"

namespace vidtk
{

vcl_string video_modality_to_string( video_modality vm )
{
  vcl_string str;
  switch( vm )
  {
    case VIDTK_COMMON:  str = "Common";      break;
    case VIDTK_EO_M:    str = "EO_MFOV";     break;
    case VIDTK_EO_N:    str = "EO_NFOV";     break;
    case VIDTK_IR_M:    str = "IR_MFOV";     break;
    case VIDTK_IR_N:    str = "IR_NFOV";     break;
    case VIDTK_EO_FB:   str = "EO_FALLBACK"; break;
    case VIDTK_IR_FB:   str = "IR_FALLBACK"; break;
    default:            str = "INVALID_TYPE"; 
  }

  return str;
}

}; //namespace vidtk
