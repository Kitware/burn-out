/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vul/vul_arg.h>
#include <vul/vul_file.h>
#include <vul/vul_sprintf.h>

#include <vnl/vnl_math.h>
#include <vnl/vnl_vector.h>

#include <vil/vil_rgb.h>
#include <vil/vil_image_view.h>
#include <vil/vil_save.h>

#include <vcl_sstream.h>
#include <vcl_algorithm.h>

#include <tracking/vsl_reader.h>
#include <tracking/kw18_reader.h>

#include <vcl_string.h>
#include <vcl_vector.h>
#include <vcl_fstream.h>

#include <tracking/track.h>
#include <tracking/track_state.h>
#include <tracking/track_initializer_process.h>

#include <vsl/vsl_binary_io.h>

vcl_vector< vidtk::track_state_sptr > getstract_start(vcl_vector< vidtk::track_state_sptr > const & h)
{
  vcl_vector< vidtk::track_state_sptr > result;
  for( unsigned int i = 0; i < 12 && i < h.size(); ++i )
  {
    result.push_back(h[i]->clone());
  }
  return result;
}

void print_speed(vidtk::track_sptr t, unsigned int i)
{
  if(i >= t->history().size()) vcl_cout << "0,";
  vcl_cout << t->history()[i]->vel_.magnitude()<<",";
}

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> trackTextArg( "--tracks-kw18", "tracks kw18" );
  vul_arg_parse( argc, argv );

  /// read the tracks
  vcl_vector< vidtk::track_sptr > tracks;
  if(trackTextArg.set())
  {
    vidtk::kw18_reader reader;
    reader.set_filename(trackTextArg().c_str());
    vcl_vector< vidtk::track_sptr > tmp;
    reader.read(tmp);
    tracks.insert(tracks.end(), tmp.begin(), tmp.end());
  }

  for( unsigned int i = 0; i < tracks.size(); ++i )
  {
    for( unsigned int j = 0; j < tracks[i]->history().size(); ++j)
    {
      vidtk::image_object_sptr ios = tracks[i]->get_object( j );
      if(!ios->bbox_.area())
      {
        vcl_cout << "SIZE IS 0 " << ios->bbox_.area() << vcl_endl;
      }
    }
  }

  return EXIT_SUCCESS;
}
