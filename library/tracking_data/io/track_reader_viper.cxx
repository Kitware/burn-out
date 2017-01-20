/*ckwg +5
 * Copyright 2012-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */


#include "track_reader_viper.h"

#include <tinyxml.h>
#include <sstream>
#include <utility>
#include <cmath>
#include <vul/vul_file.h>

#include <tracking_data/track.h>
#include <boost/algorithm/string.hpp>

#include <logger/logger.h>


namespace vidtk {
namespace ns_track_reader {

VIDTK_LOGGER("track_reader_viper");

// ----------------------------------------------------------------
track_reader_viper
::track_reader_viper()
  : been_read_(false)
{
  // Set ignore occlusions to our preferred default
  this->reader_options_.set_ignore_occlusions( false );
  this->reader_options_.set_ignore_partial_occlusions( false );
}


// ----------------------------------------------------------------
track_reader_viper
::~track_reader_viper()
{ }


// ----------------------------------------------------------------
bool
track_reader_viper
::open( std::string const& filename )
{
  if ( been_read_ )
  {
    return filename_ == filename;
  }

  filename_ = filename;

  std::string const file_ext = vul_file::extension( filename_ );
  if ( ! boost::iequals( file_ext, ".xgtf" )
       && ! boost::iequals( file_ext, ".xml") )
  {
    return false;
  }

  return file_contains( filename_, "<viper " );
}

// ----------------------------------------------------------------
bool
track_reader_viper
::read_next_terminated( vidtk::track::vector_t& datum, unsigned& frame )
{
  if ( ! been_read_)
  {
    vidtk::track::vector_t trks;
    read_all( trks );
  }

  if (this->current_terminated_ == this->terminated_at_.end() )
  {
    return false;  // end of tracks
  }

  frame = this->current_terminated_->first; // frame number
  datum = this->current_terminated_->second;

  ++this->current_terminated_;  // point to next set of tracks

  return true;
}


// ----------------------------------------------------------------
size_t
track_reader_viper
::read_all( vidtk::track::vector_t& trks )
{
  trks.clear();

  if ( been_read_ )
  {
    return 0;
  }

  descriptor_object_names_.clear();
  TiXmlDocument doc( filename_.c_str() );
  if ( doc.LoadFile() )
  {
    read_xml( &doc, trks );
  }
  else
  {
    LOG_ERROR( "Viper track reader couldn't load '" << filename_ << "'" );
    return 0;
  }

  been_read_ = true;

  // Create the terminated tracks map
  sort_terminated(trks);

  return trks.size();
}


// ----------------------------------------------------------------
void track_reader_viper
::read_xml( TiXmlNode*               pParent,
            vidtk::track::vector_t&  trks,
            XML_STATE                xml_state )
{
  if ( ! pParent )
  {
    return;
  }

  TiXmlNode* pChild;
  int t = pParent->Type();

  switch ( t )
  {
    case TiXmlNode::TINYXML_ELEMENT:
      int ival;
      //If element indicates a new object
      if ( strcmp( pParent->Value(), "object" ) == 0 )
      {
        //Create new track
        vidtk::track_sptr to_push = new vidtk::track;
        trks.push_back( to_push );

        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        int id = 0;
        while ( pAttrib )
        {
          //The head of the id is left shifted to prevent duplicate id's
          //and a base is added
          if ( strcmp( pAttrib->Name(), "id" ) == 0 )
          {
            if ( pAttrib->QueryIntValue( &ival ) == TIXML_SUCCESS )
            {
              long exponent = ( this->descriptor_object_names_.size() / 10 ) + 1;
              id += ival * static_cast< int > ( std::pow( 10l, static_cast< double > ( exponent ) ) );
            }
          }
          //The base of the id is determined by the type of object it is
          else if ( strcmp( pAttrib->Name(), "name" ) == 0 )
          {
            std::vector< std::string >::iterator iter;
            int i = 0;
            for ( iter = descriptor_object_names_.begin();
                  iter != descriptor_object_names_.end();
                  ++iter )
            {
              if ( strcmp( pAttrib->Value(), ( *iter ).c_str() ) == 0 )
              {
                id += i;
                break;
              }
              ++i;
            }
          }

          trks[trks.size() - 1]->set_id( id );
          pAttrib = pAttrib->Next();
        }
      }
      else if ( strcmp( pParent->Value(), "descriptor" ) == 0 )
      {
        xml_state = DESCRIPTOR;
        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        std::string descriptor_object_name;
        //Create a list of all descriptor object names
        while ( pAttrib )
        {
          if ( strcmp( pAttrib->Name(), "name" ) == 0 )
          {
            descriptor_object_name = pAttrib->Value();
          }
          else if ( strcmp( pAttrib->Name(), "type" ) == 0 )
          {
            if ( strcmp( pAttrib->Value(), "OBJECT" ) == 0 )
            {
              descriptor_object_names_.push_back( descriptor_object_name );
            }
          }
          pAttrib = pAttrib->Next();
        }
      }
      //If element indicates a location, occlusion, or any other
      //attribute
      else if ( ( strcmp( pParent->Value(), "attribute" ) == 0 ) && ( xml_state != DESCRIPTOR ) )
      {
        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        while ( pAttrib )
        {
          if ( strcmp( pAttrib->Value(), "Occlusion" ) == 0 )
          {
            xml_state = OCCLUSION;
          }
          else if ( strcmp( pAttrib->Value(), "Location" ) == 0 )
          {
            xml_state = LOCATION;
          }

          pAttrib = pAttrib->Next();
        }
      }
      //If the parent was an occlusion attribute
      //this fvalue will be a frame range
      else if ( ( xml_state == OCCLUSION ) && ( strcmp( pParent->Value(), "data:fvalue" ) == 0 ) )
      {
        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        while ( pAttrib )
        {
          if ( strcmp( pAttrib->Name(), "framespan" ) == 0 )
          {
            std::string str = pAttrib->Value();
            unsigned low = atoi( str.substr( 0, str.find( ":" ) ).c_str() );
            unsigned high = atoi( str.substr( str.find( ":" ) + 1 ).c_str() );

            occluded_frame_ranges.push_back( std::pair< unsigned, unsigned > ( low, high ) );
          }
          else if ( strcmp( pAttrib->Name(), "value" ) == 0 )
          {
            double dval;
            if ( pAttrib->QueryDoubleValue( &dval ) == TIXML_SUCCESS )
            {
              //Remove the occluded frame range just added if
              //it is not occluded or we want occluded values
              if ( dval == 0.0 )
              {
                occluded_frame_ranges.pop_back();
              }
              else if ( ( dval == 1.0 ) && this->reader_options_.get_ignore_partial_occlusions() )
              {
                occluded_frame_ranges.pop_back();
              }
              else if ( ( dval == 2.0 ) && this->reader_options_.get_ignore_occlusions())
              {
                occluded_frame_ranges.pop_back();
              }
            }
          }
          pAttrib = pAttrib->Next();
        }
      }
      //If parent was a location attribute
      //this will be a bounding box
      else if ( ( xml_state == LOCATION ) && ( strcmp( pParent->Value(), "data:bbox" ) == 0 ) )
      {
        TiXmlAttribute* pAttrib = pParent->ToElement()->FirstAttribute();
        int min_x = 0;
        int min_y = 0;
        int max_x = 0;
        int max_y = 0;
        int low_frame = 0;
        int high_frame = 0;
        while ( pAttrib )
        {
          //Check the frame range of the bounding box
          if ( strcmp( pAttrib->Name(), "framespan" ) == 0 )
          {
            std::string str = pAttrib->Value();
            low_frame = atoi( str.substr( 0, str.find( ":" ) ).c_str() );
            high_frame = atoi( str.substr( str.find( ":" ) + 1 ).c_str() );
          }
          //Check the dimensions of the bounding box
          else if ( pAttrib->QueryIntValue( &ival ) == TIXML_SUCCESS )
          {
            if ( strcmp( pAttrib->Name(), "height" ) == 0 )
            {
              max_y += ival;
            }
            else if ( strcmp( pAttrib->Name(), "width" ) == 0 )
            {
              max_x += ival;
            }
            else if ( strcmp( pAttrib->Name(), "x" ) == 0 )
            {
              min_x = ival;
              max_x += ival;
            }
            else if ( strcmp( pAttrib->Name(), "y" ) == 0 )
            {
              min_y = ival;
              max_y += ival;
            }
          }
          pAttrib = pAttrib->Next();
        }
        //Add the frame states to the temporary state container
        for ( int i = low_frame; i <= high_frame; ++i )
        {
          vidtk::track_state_sptr state = new track_state;

          state->time_.set_frame_number( i );
          min_x = min_x < 0 ? 0 : min_x;
          min_y = min_y < 0 ? 0 : min_y;
          max_x = max_x < 0 ? 0 : max_x;
          max_y = max_y < 0 ? 0 : max_y;

          state->amhi_bbox_.set_min_x( min_x );
          state->amhi_bbox_.set_min_y( min_y );
          state->amhi_bbox_.set_max_x( max_x );
          state->amhi_bbox_.set_max_y( max_y );

          state->loc_[0] = ( min_x + max_x ) / 2;
          state->loc_[1] = ( min_y + max_y ) / 2;
          state->loc_[2] = 0;
          temp_states.push_back( state );
        }
      }
      break;

    default:
      break;
  } // switch

    //Recurse through all the children
  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling() )
  {
    read_xml( pChild, trks, xml_state );
  }

  //If node is object, add all frames createds
  //in the recursive function above
  if ( strcmp( pParent->Value(), "object" ) == 0 )
  {
    std::vector< track_state_sptr >::iterator iter;
    //Add all states that are not occluded to trks
    for ( iter = temp_states.begin(); iter != temp_states.end(); ++iter )
    {
      std::vector< std::pair< unsigned, unsigned > >::iterator iter2;
      bool is_occluded = false;
      for ( iter2 = occluded_frame_ranges.begin(); iter2 != occluded_frame_ranges.end(); ++iter2 )
      {
        //Check if the frame is in an occluded frame range
        if ( ( ( *iter )->time_.frame_number() >= ( *iter2 ).first ) &&
             ( ( *iter )->time_.frame_number() <= ( *iter2 ).second ) )
        {
          is_occluded = true;
          break;
        }
      }
      if ( ! is_occluded )
      {
        track_state_sptr to_add = ( *iter );
        trks[trks.size() - 1]->add_state( to_add );
      }
    }
    //all valid temp states added reset value
    temp_states.clear();
    occluded_frame_ranges.clear();
  }
} // read_xml

} // end namespace
} // end namespace
