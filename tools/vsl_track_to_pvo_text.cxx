/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//vcl_track_to_pvo_text.cxx

#include <vcl_iostream.h>
#include <vcl_ostream.h>
#include <vcl_fstream.h>
#include <vcl_string.h>
#include <vul/vul_arg.h>

#include <tracking/vsl_reader.h>
#include <tracking/pvo_probability.h>

vcl_ostream &
write_out_pvo_txt( vcl_ostream & out_pvo, vcl_vector< vidtk::track_sptr > const& trks )
{
  if( ! out_pvo )
  {
    return out_pvo;
  }

  unsigned N = trks.size();
  for( unsigned i = 0; i < N; ++i )
  {
    vidtk::track_sptr track = trks[i];
    vidtk::pvo_probability pvo = track->get_pvo();
    out_pvo << track->id() << " " << pvo << vcl_endl;
  }
  out_pvo.flush();
  return out_pvo;
}

int main( int argc, char *argv[] )
{
  vul_arg<vcl_string> vslTracksArg( "--tracks", "(input) tracks vsl" );
  vul_arg<vcl_string> output_arg( "--output",
                                  "(output) text file track_id prob_person "
                                  "prob_vehicle prob_other", "out.txt");

  vul_arg_parse( argc, argv );

  if(vslTracksArg.set())
  {
    vidtk::vsl_reader reader;
    vcl_vector< vidtk::track_sptr > trks;
    reader.set_filename(vslTracksArg().c_str());
    vcl_ofstream out(output_arg().c_str());
    while(reader.read(trks))
    {
      write_out_pvo_txt( out, trks );
    }
  }

  return 0;
}
