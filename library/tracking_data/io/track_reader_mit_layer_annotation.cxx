/*ckwg +5
 * Copyright 2010-2015 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include "track_reader_mit_layer_annotation.h"

#include <utility>
#include <tinyxml.h>

#include <iostream>
#include <fstream>
#include <vul/vul_file.h>

#include <tracking_data/tracking_keys.h>
#include <boost/algorithm/string.hpp>

#include <logger/logger.h>


struct occluder
{
  std::vector< std::pair< double, double > > contour;
  int frame_number;
};


namespace {

// ----------------------------------------------------------------
bool
inside_triangle( const std::pair< double, double >& A,
                   const std::pair< double, double >& B,
                   const std::pair< double, double >& C,
                   const std::pair< double, double >& P )
{
  std::pair< double, double > v0( C.first - A.first, C.second - A.second ); // C-A
  std::pair< double, double > v1( B.first - A.first, B.second - A.second ); // B-A
  std::pair< double, double > v2( P.first - A.first, P.second - A.second ); // P-A

  //Get dot products
  ///@todo Is there some way to do this with VXL?
  double dot00 = v0.first * v0.first + v0.second * v0.second;
  double dot01 = v0.first * v1.first + v0.second * v1.second;
  double dot02 = v0.first * v2.first + v0.second * v2.second;
  double dot11 = v1.first * v1.first + v1.second * v1.second;
  double dot12 = v1.first * v2.first + v1.second * v2.second;

  double invDenom = 1 / ( dot00 * dot11 - dot01 * dot01 );
  double u = ( dot11 * dot02 - dot01 * dot12 ) * invDenom;
  double v = ( dot00 * dot12 - dot01 * dot02 ) * invDenom;

  return ( u > 0 ) && ( v > 0 ) && ( u + v < 1 );
}


// ----------------------------------------------------------------
double area_of_polygon( std::vector< std::pair< double, double > > points )
{
  size_t n = points.size();
  double to_ret = 0.0f;

  for ( size_t i = 0; i < n - 1; ++i )
  {
    to_ret += ( ( points[i].first * points[i + 1].second ) - ( points[i + 1].first * points[i].second ) );
  }
  return to_ret / 2;
}


// ----------------------------------------------------------------
bool lines_intersect( std::pair< double, double > ptA1, std::pair< double, double > ptA2,
                      std::pair< double, double > ptB1, std::pair< double, double > ptB2 )
{
  double denom = ( ( ptB2.second - ptB1.second ) * ( ptA2.first - ptA1.first ) ) -
                 ( ( ptB2.first - ptB1.first ) * ( ptA2.second - ptA1.second ) );

  if ( denom == 0 )
  {
    return false;
  }

  double ua = ( ( ( ptB2.first - ptB1.first ) * ( ptA1.second - ptB1.second ) ) -
                ( ( ptB2.second - ptB1.second ) * ( ptA1.first - ptB1.first ) ) ) / denom;

  double ub = ( ( ( ptA2.first - ptA1.first ) * ( ptA1.second - ptB1.second ) ) -
                ( ( ptA2.second - ptA1.second ) * ( ptA1.first - ptB1.first ) ) ) / denom;

  if ( ( ua < 0 ) || ( ua > 1 ) || ( ub < 0 ) || ( ub > 1 ) )
  {
    return false;
  }

  return true;
}


// ----------------------------------------------------------------
void triangulate( std::vector< occluder >& contours, std::vector< occluder >& triangles )
{
  std::vector< occluder >::iterator iter;

  for ( iter = contours.begin(); iter != contours.end(); ++iter )
  {
    std::vector< std::pair< double, double > > occl = ( *iter ).contour;
    int n = occl.size();
    double a = area_of_polygon( occl );

    for ( int i = 0; n > 2; i = ( i + 1 ) % n )
    {
      bool is_ear = true;
      //get adjacent indecies
      int im1 = i == 0 ? n - 1 : i - 1; // i-1 mod n
      int ip1 = ( i + 1 ) % n; // i+1 mod n

      std::pair< double, double > unit_vec_1( occl[ip1].first - occl[i].first,
                                             occl[ip1].second - occl[i].second );

      std::pair< double, double > unit_vec_2( occl[im1].first - occl[i].first,
                                             occl[im1].second - occl[i].second );


      double crossprod = ( ( occl[i].first - occl[im1].first ) * ( occl[ip1].second - occl[i].second )
                           - ( occl[i].second - occl[im1].second ) * ( occl[ip1].first - occl[i].first ) );

      bool convex = crossprod * a > 0;

      if ( n > 3 )
      {
        if ( ! convex )
        {
          is_ear = false;
          continue;
        }

        for ( int j = 0; j < n; ++j )
        {
          //Compare triangle I with all points
          if ( ( j == i ) || ( j == im1 ) || ( j == ip1 ) )
          {
            continue;
          }

          if ( inside_triangle( occl[im1], occl[i], occl[ip1], occl[j] ) )
          {
            is_ear = false;
            break;
          }
        }
      }
      if ( is_ear )
      {
        //Add ear to the triangle list
        occluder ear;
        ear.contour.push_back( occl[im1] );
        ear.contour.push_back( occl[i] );
        ear.contour.push_back( occl[ip1] );
        ear.frame_number = ( *iter ).frame_number;
        triangles.push_back( ear );
        //Remove point i from the contour
        occl.erase( occl.begin() + i );
        n -= 1;
        i -= 1;

        //added last triangle move to next contour
        if ( n == 2 )
        {
          break;
        }
      }
    }
  }
} // triangulate


// ----------------------------------------------------------------
bool is_occluded( double min_x,
                  double min_y,
                  double max_x,
                  double max_y,
                  double frame_index,
                  std::vector< occluder >* occluders )
{
  bool occluded = false;

  std::pair< double, double > corner1( min_x, min_y );
  std::pair< double, double > corner2( max_x, min_y );
  std::pair< double, double > corner3( max_x, max_y );
  std::pair< double, double > corner4( min_x, max_y );
  std::vector< occluder >::iterator iter;


  //Check if any tracked point is in an occluder
  //or any occlder point is in a track the track
  //is occluded
  for ( iter = occluders->begin(); iter != occluders->end(); ++iter )
  {
    //don't bother checking if it is not in the same frame
    if ( frame_index != iter->frame_number )
    {
      continue;
    }

     //Check if any triangle points are in the bounding box
    std::vector< std::pair< double, double > >::iterator iter2;
    for ( iter2 = iter->contour.begin(); iter2 != iter->contour.end(); ++iter2 )
    {
      if ( ( iter2->first > min_x ) && ( iter2->first < max_x ) &&
           ( iter2->second > min_y ) && ( iter2->second < max_y ) )
      {
        occluded = true;
        break;
      }
    }
    //Check to see if any lines on the objects are intersecting
    if ( ! occluded )
      for ( int i = 0; i < 3 && ! occluded; ++i )
      {
        occluded =
          lines_intersect( corner1, corner2, iter->contour[i], iter->contour[( i + 1 ) % 3] ) ||
          lines_intersect( corner2, corner3, iter->contour[i], iter->contour[( i + 1 ) % 3] ) ||
          lines_intersect( corner3, corner4, iter->contour[i], iter->contour[( i + 1 ) % 3] ) ||
          lines_intersect( corner4, corner1, iter->contour[i], iter->contour[( i + 1 ) % 3] );
      }
     //Check if any bounding box points are in the triangle
    if ( occluded ||
         inside_triangle(
           iter->contour[0],
           iter->contour[1],
           iter->contour[2],
           corner1 ) ||
         inside_triangle(
           iter->contour[0],
           iter->contour[1],
           iter->contour[2],
           corner2 ) ||
         inside_triangle(
           iter->contour[0],
           iter->contour[1],
           iter->contour[2],
           corner3 ) ||
         inside_triangle(
           iter->contour[0],
           iter->contour[1],
           iter->contour[2],
           corner4 ) )
    {
      occluded = true;
      break;
    }

  }
  return occluded;
} // is_occluded


} // end anon namespace


// ================================================================

namespace vidtk {
namespace ns_track_reader {

VIDTK_LOGGER("track_reader_mit_layer_annotation");


// ----------------------------------------------------------------
track_reader_mit_layer_annotation
::track_reader_mit_layer_annotation()
  : been_read_(false)
{
  global_min_x = -1;
  global_min_y = -1;
  global_max_x = -1;
  global_max_y = -1;

  global_frame_index = -1;

  frame_indexing = 0;
  frequency_of_frames = -1;

  // Set ignore occlusions to our preferred default
  this->reader_options_.set_ignore_occlusions( true );
  this->reader_options_.set_ignore_partial_occlusions( true );
 }


track_reader_mit_layer_annotation
::~track_reader_mit_layer_annotation()
{ }


// ----------------------------------------------------------------
bool track_reader_mit_layer_annotation
::open( std::string const& filename )
{
  // Get frequency from config options if set in options.
  if (this->reader_options_.is_sec_per_frame_set() )
  {
    frequency_of_frames = this->reader_options_.get_sec_per_frame();
  }

  // determine if file can be opened
  if ( been_read_ )
  {
    return filename_ == filename;
  }

  filename_ = filename;

  std::string const file_ext = vul_file::extension( filename_ );
  if ( ! boost::iequals( file_ext, ".mit" )
       && ! boost::iequals( file_ext, ".xml") )
  {
    return false;
  }

  return file_contains( filename_, "<annotation>" );
}


// ----------------------------------------------------------------
bool
track_reader_mit_layer_annotation
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
track_reader_mit_layer_annotation
::read_all( vidtk::track::vector_t& trks )
{
  std::vector< occluder > triangulated_occluders;
  if ( ( ! this->reader_options_.get_occluder_filename().empty() )
       && ( ! this->reader_options_.get_ignore_occlusions()) )
  {
    std::vector< occluder > occluders;
    TiXmlDocument oc_doc( this->reader_options_.get_occluder_filename().c_str() );
    if ( oc_doc.LoadFile() )
    {
      read_occulder_xml( &oc_doc, occluders );
    }

    triangulate( occluders, triangulated_occluders );
  }

  trks.clear();
  if ( been_read_ )
  {
    return 0;
  }

  TiXmlDocument doc( filename_.c_str() );
  if ( doc.LoadFile() )
  {
    if ( this->reader_options_.get_ignore_occlusions() )
    {
      read_xml( &doc, trks, 0 );
    }
    else
    {
      read_xml( &doc, trks, &triangulated_occluders );
    }
  }
  else
  {
    LOG_ERROR( "MIT layer annotation reader couldn't load '" << filename_ << "'" );
    return 0;
  }

  // Create the terminated tracks map
  sort_terminated(trks);

  been_read_ = true;
  return trks.size();
}


// ----------------------------------------------------------------
void
track_reader_mit_layer_annotation
::read_occulder_xml( TiXmlNode*               pParent,
                     std::vector< occluder >&  occluders )
{
  if ( ! pParent )
  {
    return;
  }

  TiXmlNode* pChild;
  TiXmlText* pText;
  int t = pParent->Type();

  switch ( t )
  {
    case TiXmlNode::TINYXML_ELEMENT:
      // Frames and objects indicate that the last
      // bounding box has been competely read in
      if ( strcmp( pParent->Value(), "frame" ) == 0 )
      {
        occluder topush;
        occluders.push_back( topush );
      }
      prev_element = pParent->Value();
      break;

    case TiXmlNode::TINYXML_TEXT:
      const char* pe;
      //The previous element indicates what this text node will be
      pe = prev_element.c_str();
      pText = pParent->ToText();

      if ( strcmp( pe, "notes" ) == 0 )
      {
        //This should be expanded
      }
      else if ( strcmp( pe, "index" ) == 0 )
      {
        occluders[occluders.size() - 1].frame_number = atoi( pText->Value() );
      }
      //Create a bounding box from the polygon in the xml file
      else if ( strcmp( pe, "x" ) == 0 )
      {
        std::pair< double, double > pt;
        pt.first = atof( pText->Value() );
        occluders[occluders.size() - 1].contour.push_back( pt );
      }
      else if ( strcmp( pe, "y" ) == 0 )
      {
        occluders[occluders.size() - 1].contour[occluders[occluders.size() - 1].contour.size() - 1].second = atof( pText->Value() );
      }
      break;

    default:
      break;
  } // switch

  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling() )
  {
    this->read_occulder_xml( pChild, occluders );
  }

} // read_occulder_xml


// ----------------------------------------------------------------
void track_reader_mit_layer_annotation
::read_xml( TiXmlNode*                        pParent,
              std::vector< vidtk::track_sptr >&  trks,
              std::vector< occluder >*           occluders = 0 )
{
  if ( ! pParent )
  {
    return;
  }

  TiXmlNode* pChild;
  TiXmlText* pText;
  int t = pParent->Type();

  switch ( t )
  {
    case TiXmlNode::TINYXML_ELEMENT:

      //Reading a new object push a new track and
      //Reset any values that need reseting
      if ( strcmp( pParent->Value(), "object" ) == 0 )
      {
        vidtk::track_sptr topush = new vidtk::track;
        trks.push_back( topush );
      }
      prev_element = pParent->Value();
      break;

    case TiXmlNode::TINYXML_TEXT:
      const char* pe;
      //The previous element indicates what this text node will be
      pe = prev_element.c_str();
      pText = pParent->ToText();

      if ( strcmp( pe, "id" ) == 0 )
      {
        trks[trks.size() - 1]->set_id( atoi( pText->Value() ) );
      }
      else if ( strcmp( pe, "notes" ) == 0 )
      {
        //This should be expanded
      }
      else if ( strcmp( pe, "index" ) == 0 )
      {
        global_frame_index = atoi( pText->Value() );
      }
      //Create a bounding box from the polygon in the xml file
      else if ( strcmp( pe, "x" ) == 0 )
      {
        global_min_x = ( global_min_x < atof( pText->Value() )
                         && global_min_x != -1 ) ? global_min_x : atof( pText->Value() );
        global_max_x = ( global_max_x > atof( pText->Value() )
                         && global_max_x != -1 ) ? global_max_x : atof( pText->Value() );
        //make sure there are no negative coordinates
        global_min_x = global_min_x < 0 ? 0 : global_min_x;
        global_max_x = global_max_x < 0 ? 0 : global_max_x;
      }
      else if ( strcmp( pe, "y" ) == 0 )
      {
        global_min_y = ( global_min_y < atof( pText->Value() )
                         && global_min_y != -1 ) ? global_min_y : atof( pText->Value() );
        global_max_y = ( global_max_y > atof( pText->Value() )
                         && global_max_y != -1 ) ? global_max_y : atof( pText->Value() );
        //make sure there are no negative coordinates
        global_min_y = global_min_y < 0 ? 0 : global_min_y;
        global_max_y = global_max_y < 0 ? 0 : global_max_y;
      }
      break;

    default:
      break;
  } // switch

  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling() )
  {
    read_xml( pChild, trks, occluders );
  }

  //Add a New Frame
  if ( strcmp( pParent->Value(), "frame" ) == 0 )
  {
    vnl_vector_fixed< double, 3 > loc;

    // Calculate the center of the bounding box
    loc[0] = ( global_min_x + global_max_x ) / 2.0;
    loc[1] = ( global_min_y + global_max_y ) / 2.0;
    loc[2] = 0;

    unsigned min[2];
    unsigned max[2];

    track_state_sptr to_add = new track_state;
    to_add->loc_[0] = loc[0];
    to_add->loc_[1] = loc[1];
    to_add->loc_[2] = loc[2];
    to_add->time_.set_frame_number( global_frame_index + frame_indexing );
    if ( frequency_of_frames != -1 )
    { //time_ is a timestamp which stores time in microseconds
      //(frame number/frame frequency) is in seconds
      //so, convert from seconds to microseconds
      double tm = ( ( global_frame_index + frame_indexing + 1 ) / frequency_of_frames ) * 1e6;
      to_add->time_.set_time( tm );
    }
    min[0] = static_cast< unsigned > ( global_min_x );
    min[1] = static_cast< unsigned > ( global_min_y );
    max[0] = static_cast< unsigned > ( global_max_x );
    max[1] = static_cast< unsigned > ( global_max_y );

    vgl_box_2d< unsigned > bbox = vgl_box_2d< unsigned > ( min, max );

    image_object_sptr obj = new image_object;
    obj->set_image_loc( loc[0], loc[1] );
    obj->set_bbox(
      static_cast< unsigned > ( global_min_x ),
      static_cast< unsigned > ( global_max_x ),
      static_cast< unsigned > ( global_min_y ),
      static_cast< unsigned > ( global_max_y )
      );

    obj->set_area(( global_max_x - global_min_x ) * ( global_max_y - global_min_y ));
    to_add->data_.set( tracking_keys::img_objs,
                       std::vector< image_object_sptr > ( 1, obj ) );
    to_add->amhi_bbox_ = bbox;

    if ( occluders == 0 )
    {
      trks.back()->add_state( to_add );
    }
    else if ( ! is_occluded( global_min_x, global_min_y, global_max_x, global_max_y, global_frame_index, occluders ) )
    {
      trks.back()->add_state( to_add );
    }

     //Reset bbox temp variables
    global_min_x = -1;
    global_min_y = -1;
    global_max_x = -1;
    global_max_y = -1;
  }
} // read_xml

} // end namespace
} // end namespace
