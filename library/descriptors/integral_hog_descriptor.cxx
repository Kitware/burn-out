/*ckwg +5
 * Copyright 2013-2014 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "integral_hog_descriptor.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <fstream>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include <logger/logger.h>

VIDTK_LOGGER( "integral_hog_descriptor" );

namespace vidtk
{

// Return number of features in this custom_ii_hog descriptor
unsigned custom_ii_hog_model::feature_count() const
{
  unsigned feat_count = 0;

  for( unsigned i = 0; i < blocks_.size(); i++ )
  {
    feat_count += blocks_[i].size() * bins_per_cell;
  }

  return feat_count;
}

inline bool is_in_range( double input, double lower, double upper )
{
  return ( input >= lower && input <= upper );
}

bool custom_ii_hog_model::is_valid() const
{
  // Check to make sure cells are all in the right range
  if( cells_.size() == 0 )
  {
    return false;
  }
  for( unsigned i = 0; i < cells_.size(); i++ )
  {
    if( !is_in_range( cells_[i].min_x(), 0.0, 1.0 ) ||
        !is_in_range( cells_[i].max_x(), 0.0, 1.0 ) ||
        !is_in_range( cells_[i].min_y(), 0.0, 1.0 ) ||
        !is_in_range( cells_[i].max_y(), 0.0, 1.0 ) )
    {
      LOG_ERROR( "Cell location must be in range [0,1]" );
      return false;
    }
  }
  // Check to make sure all blocks reference valid cells
  for( unsigned i = 0; i < blocks_.size(); i++ )
  {
    for( unsigned j = 0; j < blocks_[i].size(); j++ )
    {
      if( blocks_[i][j] >= cells_.size() )
      {
        LOG_ERROR( "Block index exceeds number of cells" );
        return false;
      }
    }
  }
  return true;
}

// Load a custom model from a file
bool custom_ii_hog_model::load_model( const std::string& filename )
{
  this->clear();

  try
  {
    // Open file
    std::ifstream input( filename.c_str() );

    if( !input )
    {
      LOG_ERROR( "Unable to open " << filename << "!" );
      return false;
    }

    // Parse file header, if it exists
    unsigned cell_entries, block_entries;
    std::string header;
    input >> header;

    if( header == "HOG_MODEL_FILE" || header == "ASCII_HOG_FILE" )
    {
      input >> cell_entries >> block_entries;
    }
    else
    {
      cell_entries = boost::lexical_cast<unsigned>( header );
      input >> block_entries;
    }

    if( cell_entries == 0 )
    {
      LOG_ERROR( "HoG file " << filename << " contains invalid header information." );
      return false;
    }

    // Read in model
    std::string line, type_str, id_str;
    while( std::getline(input, line) )
    {
      std::istringstream ss( line );
      ss >> type_str >> id_str;
      if( type_str == "CELL" )
      {
        double lx, ly, ux, uy;
        ss >> lx >> ly >> ux >> uy;
        cells_.push_back( vgl_box_2d< double >( lx, ux, ly, uy ) );
      }
      else if( type_str == "BLOCK" )
      {
        blocks_.push_back( std::vector< unsigned >() );
        while( ss )
        {
          unsigned ind;
          ss >> ind;
          blocks_[ blocks_.size()-1 ].push_back( ind-1 );
        }
      }
      else
      {
        LOG_WARN( "Invalid string in HoG File: " << type_str );
      }
    }

    // Confirm size matches header
    if( cells_.size() != cell_entries || blocks_.size() != block_entries )
    {
      LOG_ERROR( "HoG model header info does not match data." );
      return false;
    }

    // Close file
    input.close();
  }
  catch( const std::ifstream::failure& e )
  {
    LOG_ERROR( "Unable to read " << filename << ", " << e.what() );
    return false;
  }

  // Make sure inputted values are valid
  if( !this->is_valid() )
  {
    LOG_ERROR( "Inputted model file contains flaw." );
    return false;
  }
  return true;
}

} // end namespace vidtk
