/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_arg.h>
#include <vcl_string.h>
#include <vcl_fstream.h>

#include <vpgl/vpgl_lvcs.h>


int main(int argc, char *argv[])
{
  vul_arg<vcl_string> in_geo( "--in-geo", "input geo coordinates (long,lat,alt)", "" );
  vul_arg<vcl_string> out_geo( "--out-geo", "output geo coordinates (long,lat,alt)", "" );
  vul_arg<vcl_string> in_lvc( "--in-lvc", "input local vertical coordinates (x,y,z)", "" );
  vul_arg<vcl_string> out_lvc( "--out-lvc", "output local vertical coordinates (x,y,z)", "" );
  vul_arg<unsigned>   model( "--model", "Earth model (0=WGS84, 1=NAD27N, 2=WGS72)", 0);
  vul_arg<double>  origin_long( "--o-long", "longitude of local coordinates origin (degrees)", 0);
  vul_arg<double>  origin_lat( "--o-lat", "latitude of local coordinates origin (degrees)", 0);
  vul_arg<double>  origin_alt( "--o-alt", "altitude of local coordinates origin (meters)", 0);

  vul_arg_parse( argc, argv );

  vpgl_lvcs lvcs(origin_lat(), origin_long(), origin_alt(), 
                 vpgl_lvcs::cs_names(model()), vpgl_lvcs::DEG, vpgl_lvcs::METERS);

  if (in_geo.set() && out_lvc.set())
  {
    vcl_ifstream ifs(in_geo().c_str());
    vcl_ofstream ofs(out_lvc().c_str());
    double lon, lat, alt, x, y, z;
    while( ifs >> lon >> lat >> alt )
    {
      lvcs.global_to_local(lon, lat, alt, vpgl_lvcs::cs_names(model()),
                           x, y, z);
      ofs <<x<< " "<<y<<" "<<z<<vcl_endl;
    }
    ifs.close();
    ofs.close();
  }
  else if (in_lvc.set() && out_geo.set())
  {
    vcl_ifstream ifs(in_lvc().c_str());
    vcl_ofstream ofs(out_geo().c_str());
    double lon, lat, alt, x, y, z;
    while( ifs >> x >> y >> z )
    {
      lvcs.local_to_global(x, y, z, vpgl_lvcs::cs_names(model()),
                           lon, lat, alt);
      ofs <<lon<< " "<<lat<<" "<<alt<<vcl_endl;
    }
    ifs.close();
    ofs.close();
  }
  else
  {
    vcl_cout << "must specify --in-geo and --out-lvc or\n" 
             << "             --in-lvc and --out-geo" <<vcl_endl;
    return -1;
  }

  return 0;
}
