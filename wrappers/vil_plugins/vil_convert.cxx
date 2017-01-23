/*ckwg +5
 * Copyright 2013-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <string>
#include <limits>

#include <vul/vul_arg.h>
#include <vil/vil_image_resource.h>
#include <vil/vil_load.h>
#include <vil/vil_save.h>

#include "vil_plugin_loader.h"
#include <vil_plugins/vil_plugins_config.h>
#if defined(HAS_LTI_NITF2) || defined(HAS_LTI_J2K)
#include "file_formats/vil_lti_base_image.h"
#endif
#if defined(HAS_LTI_NITF2) || defined(HAS_VIL_GDAL)
#include "file_formats/vil_nitf_engrda.h"
#endif

int main(int argc, char** argv)
{
  vul_arg<std::string> arg_in("-i", "Input file", "");
  vul_arg<std::string> arg_out("-o", "Output file", "");
  vul_arg<unsigned>   arg_i0("-i0", "Upper left corner of crop scene", 0);
  vul_arg<unsigned>   arg_j0("-j0", "Upper left corner of crop scene", 0);
  vul_arg<unsigned>   arg_ni("-ni", "Width of crop scene (default is whole image)", std::numeric_limits<unsigned>::max());
  vul_arg<unsigned>   arg_nj("-nj", "Width of crop scene (default is whole imag))", std::numeric_limits<unsigned>::max());
  vul_arg<unsigned>   arg_t("-t", "Number of decoding threads to use (if supported)", 1);

  vul_arg_parse(argc, argv);
  if(arg_in() == "")
  {
    std::cerr << "ERROR: No input file specified" << std::endl;
    return EXIT_FAILURE;
  }

#if defined(HAS_LTI_NITF2) || defined(HAS_LTI_J2K)
  vil_lti_base_image::set_num_threads(arg_t());
#endif

  std::clog << "Loading vil plugins..." << std::flush;
  vidtk::load_vil_plugins();
  std::clog << "done." << std::endl;

  std::clog << "Loading image resource " << arg_in() << "..." << std::flush;
  vil_image_resource_sptr im = vil_load_image_resource(arg_in().c_str());
  if(!im)
  {
    std::cerr << "Error" << std::endl;
    return EXIT_FAILURE;
  }
  std::clog << "done" << std::endl;
  std::clog << "Image " << im->ni() << "x" << im->nj() << "x" << im->nplanes()
           << std::endl;

  unsigned i0 = arg_i0();
  unsigned j0 = arg_j0();
  unsigned ni = arg_ni() == std::numeric_limits<unsigned>::max() ? im->ni() : arg_ni();
  unsigned nj = arg_nj() == std::numeric_limits<unsigned>::max() ? im->nj() : arg_nj();

  std::string fmt(im->file_format());
  std::clog << "File Format: " << fmt << std::endl;

  if(fmt == "j2k")
  {
    std::clog << "Decoding metadata..." << std::endl;
    std::clog << std::fixed << std::setprecision(6);
    double xy[2];
    if(!im->get_property("UL", xy))
    {
      std::cerr << "Error retrieving property UL" << std::endl;
    }
    else
    {
      std::clog << "UL: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("UR", xy))
    {
      std::cerr << "Error retrieving property UR" << std::endl;
    }
    else
    {
      std::clog << "UR: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("LR", xy))
    {
      std::cerr << "Error retrieving property LR" << std::endl;
    }
    else
    {
      std::clog << "LR: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("LL", xy))
    {
      std::cerr << "Error retrieving property LL" << std::endl;
    }
    else
    {
      std::clog << "LL: " << xy[0] << " " << xy[1] << std::endl;
    }
  }
  else if(fmt == "nitf2" || fmt == "GDALReadable")
  {
    std::clog << "Decoding metadata..." << std::endl;
    std::clog << std::fixed << std::setprecision(6);
    double xy[2];

    if(!im->get_property("FRFC_LOC", xy))
    {
      std::cerr << "Error retrieving property FRFC_LOC" << std::endl;
    }
    else
    {
      std::clog << "FRFC_LOC: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("FRLC_LOC", xy))
    {
      std::cerr << "Error retrieving property FRLC_LOC" << std::endl;
    }
    else
    {
      std::clog << "FRLC_LOC: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("LRLC_LOC", xy))
    {
      std::cerr << "Error retrieving property LRLC_LOC" << std::endl;
    }
    else
    {
      std::clog << "LRLC_LOC: " << xy[0] << " " << xy[1] << std::endl;
    }
    if(!im->get_property("LRFC_LOC", xy))
    {
      std::cerr << "Error retrieving property LRFC_LOC" << std::endl;
    }
    else
    {
      std::clog << "LRFC_LOC: " << xy[0] << " " << xy[1] << std::endl;
    }
  }
#if defined(HAS_LTI_NITF2) || defined(HAS_VIL_GDAL)
  if(fmt == "nitf2" || fmt == "GDALReadable")
  {
    std::clog << "Decoding NITF metadata..." << std::endl;
    std::clog << std::fixed << std::setprecision(6);
    std::time_t t;
    if(!im->get_property("IDATIM", &t))
    {
      std::cerr << "Error retrieving property IDATIM" << std::endl;
    }
    else
    {
      std::tm *utc = std::gmtime(&t);
      std::clog << "IDATIM: " << asctime(utc) << std::flush;
    }

    vil_nitf_engrda engrda;
    if(!im->get_property("ENGRDA", &engrda))
    {
      std::cerr << "Error retrieving property ENGRDA" << std::endl;
    }
    else
    {
      std::clog << "ENGRDA: " << std::endl;
      for(vil_nitf_engrda::const_iterator i = engrda.begin();
          i != engrda.end();
          ++i)
      {
        std::string value;
        std::string name = i->second->get_label();
        std::string units = i->second->get_units();
        i->second->get_data(value);
        std::clog << std::setw(18) << name << " : "
                 << value << (units == "NA" ? "" : units) << std::endl;
      }
    }
  }
#endif

  std::clog << "Decoding view (" << i0 << "," << j0 << ") "
                         << "(+" << ni << ",+" << nj << ")..." << std::flush;

  vil_image_view_base_sptr view = im->get_copy_view(i0, ni, j0, nj);
  if(!view)
  {
    std::cerr << "Error" << std::endl;
    return EXIT_FAILURE;
  }
  std::clog << "done" << std::endl;

  if(arg_out() != "")
  {
    std::clog << "Saving " << arg_out() << "..." << std::flush;
    if(!vil_save(*view, arg_out().c_str()))
    {
      std::cerr << "Error" << std::endl;
      return EXIT_FAILURE;
    }
    std::clog << "done" << std::endl;
  }

  return EXIT_SUCCESS;
}
