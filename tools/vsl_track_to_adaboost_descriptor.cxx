/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

//vsl_track_to_adaboost_descriptor.cxx

#include <classifier/track_to_vector_of_descriptors.h>

#include <vul/vul_arg.h>

#include <vnl/vnl_random.h>
#include <vnl/vnl_math.h>

#include <vcl_iostream.h>
#include <vcl_vector.h>
#include <vcl_string.h>
#include <vcl_algorithm.h>
#include <vcl_sstream.h>
#include <vcl_iomanip.h>
#include <vcl_fstream.h>
#include <iostream>
#include <algorithm>
#include <functional>
#include <vector>
#include <ctime>
#include <cstdlib>

#include <tracking/vsl_reader.h>

#include <vsl/vsl_vector_io.h>

#include <vsl/vsl_binary_io.h>

bool write_vector(vcl_string fname, vcl_vector<double> const & vect)
{
  vsl_b_ofstream in(fname);
  if(!in)
  {
    return false;
  }
  vsl_b_write(in, vect);
  return true;
}

int main( int argc, char *argv[] )
{

  vul_arg<vcl_string> trackBinaryFileVSLArg( "--track-vsl", "VIDTK binary tracks" );
  vul_arg<vcl_string> outprefixArg("--outprefix", "The prefix of output of classifying tracks", "out");
  vul_arg_parse( argc, argv );

  vcl_ofstream out( (outprefixArg()+"_desc.txt").c_str() );

  vidtk::track_to_vector_of_descriptors ttvod;

  if(trackBinaryFileVSLArg.set())
  {
    vidtk::vsl_reader reader;
    vcl_vector< vidtk::track_sptr > trks;
    reader.set_filename(trackBinaryFileVSLArg().c_str());
    while(reader.read(trks))
    {
      for( unsigned int i = 0; i < trks.size(); ++i )
      {
        vnl_vector<double> v = ttvod.generateData( trks[i] )->vectorize();
        out /*<< trks[i]->id() << "\t"*/ << v << vcl_endl;
      }
    }
  }

  return 0;
}
