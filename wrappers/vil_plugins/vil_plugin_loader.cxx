/*ckwg +5
 * Copyright 2013 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include "vil_plugin_loader.h"

#include <vil_plugins/vil_plugins_config.h>

#ifdef HAS_LTI_NITF2
  #include "file_formats/vil_lti_nitf2_image.h"
#endif
#ifdef HAS_LTI_J2K
  #include "file_formats/vil_lti_j2k_image.h"
#endif
#ifdef HAS_VIL_GDAL
#include "file_formats/vil_gdal_image.h"
#endif

namespace vidtk
{

void load_vil_plugins(void)
{
  static bool is_loaded = false;
  if(!is_loaded)
  {
#ifdef HAS_LTI_NITF2
    vil_file_format::add_file_format(new vil_lti_nitf2_file_format);
#endif
#ifdef HAS_LTI_J2K
    vil_file_format::add_file_format(new vil_lti_j2k_file_format);
#endif
#ifdef HAS_VIL_GDAL
    vil_file_format::add_file_format(new vil_gdal_file_format);
#endif
    is_loaded = true;
  }
}

}
