/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <vcl_fstream.h>
#include <utilities/unchecked_return_value.h>

#include "multiple_features.h"
#include <utilities/log.h>

namespace vidtk
{
multiple_features
::multiple_features()
:tangential_sigma( 0.0 ),
 normal_sigma( 0.0 ),
 test_enabled( false ), 
 train_enabled( false ),
 w_color( 0.0 ),
 w_kinematics( 0.0 ),
 w_area( 0.0 ),
 pdf_delta( 0.0 ),
 min_prob_for_log( 0.0 ),
 cdfs(),
 train_filename()
{
  cdfs.fill( 0.0 );
}

multiple_features
::~multiple_features()
{
}


double 
multiple_features
::get_similarity_score( double in_prob, Multiple_Feature_Types type_idx ) const
{
  unsigned idx = type_idx * 2;

  //Values in the CDF are already sorted in ascending order. 

  //max_value - min_value
  double range = cdfs( cdfs.rows() - 1, idx) - cdfs(0, idx);
  unsigned bin_idx = unsigned(vcl_floor( ( (in_prob - cdfs(0, idx))/ range ) 
                                           * (cdfs.rows()-1) ) );
  return cdfs( bin_idx, idx+1);
}

bool 
multiple_features
::load_cdfs_params( vcl_string cdfs_filename )
{
  vcl_ifstream cdfs_file;

  try
  {
    cdfs_file.open( cdfs_filename.c_str() );
    if( ! cdfs_file )
    {
      throw unchecked_return_value( "Couldn't open cdf file." );
    }
    else
    {
      cdfs_file >> normal_sigma;
      cdfs_file >> tangential_sigma;
      cdfs_file >> min_prob_for_log;
      cdfs_file >> bin_count;
      unsigned cols;
      cdfs_file >> cols;

      cdfs.set_size( bin_count, cols );

      for( unsigned i = 0; i < bin_count; ++i )
      {
        for( unsigned j = 0; j < cols; ++j )
        {
          if( ! (cdfs_file >> cdfs(i,j)) )
          {
            if( cdfs_file.eof() && j == bin_count-1)
            {
              cdfs_file.close();
              return true;
            }
            else
            {
              throw unchecked_return_value( "Problem loading cdfs file." );
            }
          }
        }
      }
    }
  }
  catch( unchecked_return_value& e )
  {
    log_error( "multiple_features::load_cdfs() failed: "
      << e.what() << "\n" );
    cdfs_file.close();
    return false;
  }

  cdfs_file.close();

  return true;
}

}// namespace vidtk
