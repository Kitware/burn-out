/*ckwg +5
 * Copyright 2010 by Kitware, Inc. All Rights Reserved. Please refer to
 * KITWARE_LICENSE.TXT for licensing information, or contact General Counsel,
 * Kitware, Inc., 28 Corporate Drive, Clifton Park, NY 12065.
 */

#include <tracking/mit_layer_annotation_reader.h>
#include <vcl_utility.h>
#include <tinyxml.h>

#include <vcl_iostream.h>
#include <vcl_fstream.h>

#include <tracking/tracking_keys.h>

using namespace vidtk;

//Temporary variables used to keep track of previous values
//while reading the xml file

vcl_string prev_element;

double min_x=-1;
double min_y=-1;
double max_x=-1;
double max_y=-1;

int frame_index = -1;

struct occluder
{
  vcl_vector< vcl_pair<double,double> > contour;
  int frame_number;
};


void write_triangle_to_file(vcl_pair<double,double>& p1,
                            vcl_pair<double,double>& p2,
                            vcl_pair<double,double>& p3,
                            int frame_number,
                            vcl_string const& f
                            )
{
  static int id = 0;
  ++id;
  vcl_ofstream of(f.c_str(), vcl_ios::app);
  of << "\n"
     << "<object>\n"
     << "  <name>" << id-1 << "</name>\n"
     << "  <notes></notes>\n"
     << "  <deleted>0</deleted>\n"
     << "  <verified>0</verified>\n"
     << "  <date>11-Jan-2010 19:08:30</date>\n"
     << "  <id>11" << id << "</id>\n"
     << "  <username>anonymous</username>\n"
     << "  <numFrames>653</numFrames>\n"
     << "  <startFrame>" << frame_number << "</startFrame>\n"
     << "  <endFrame>" << frame_number << "</endFrame>\n"
     << "  <createdFrame>0</createdFrame>\n"
     << "  <labels>\n"
     << "    <frame>\n"
     << "       <index>" << frame_number << "</index>\n"
     << "       <depth>0</depth>\n"
     << "       <globalLabel>1</globalLabel>\n"
     << "       <localLabel>1</localLabel>\n"
     << "       <tracked>1</tracked>\n"
     << "       <depthLabel>1</depthLabel>\n"
     << "       <isPolygon>1</isPolygon>\n"
     << "       <polygon>\n"
     << "         <pt>\n"
     << "           <x>" << p1.first << " </x>\n"
     << "           <y>" << p1.second << "</y>\n"
     << "           <labeled>1</labeled>\n"
     << "         </pt>\n"
     << "         <pt>\n"
     << "           <x>" << p2.first << "</x>\n"
     << "           <y>" << p2.second << "</y>\n"
     << "           <labeled>1</labeled>\n"
     << "         </pt>\n"
     << "           <pt>\n"
     << "           <x>" << p3.first << "</x>\n"
     << "           <y>" << p3.second << "</y>\n"
     << "           <labeled>1</labeled>\n"
     << "         </pt>\n"
     << "       </polygon>\n"
     << "     </frame>\n"
     << "   </labels>\n"
     << "</object>\n";
  of.close();         
}

bool inside_triangle(const vcl_pair<double,double>& A,
                     const vcl_pair<double,double>& B,
                     const vcl_pair<double,double>& C,
                     const vcl_pair<double,double>& P)
{
  vcl_pair<double,double> v0(C.first - A.first, C.second - A.second); // C-A
  vcl_pair<double,double> v1(B.first - A.first, B.second - A.second); // B-A
  vcl_pair<double,double> v2(P.first - A.first, P.second - A.second); // P-A

  //Get dot products
  double dot00 = v0.first * v0.first + v0.second * v0.second;
  double dot01 = v0.first * v1.first + v0.second * v1.second;
  double dot02 = v0.first * v2.first + v0.second * v2.second;
  double dot11 = v1.first * v1.first + v1.second * v1.second;
  double dot12 = v1.first * v2.first + v1.second * v2.second;

  double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
  double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
  double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

  return (u > 0) && (v > 0) && (u + v < 1);
}

double area_of_polygon(vcl_vector< vcl_pair<double,double> > points)
{
  int n = points.size();
  double to_ret = 0.0f;
  for(int i = 0; i < n-1 ; ++i)
  {
    to_ret += ((points[i].first*points[i+1].second)-(points[i+1].first*points[i].second));
  }
  return to_ret/2;
}

void triangulate(vcl_vector<occluder> & contours, vcl_vector<occluder>& triangles)
{
  vcl_vector<occluder>::iterator iter;
  for(iter = contours.begin(); iter != contours.end(); ++iter)
  {
    vcl_vector< vcl_pair<double,double> > occl = (*iter).contour;
    int n = occl.size();
    double a = area_of_polygon(occl); 
    for(int i = 0; n > 2; i=(i+1)%n)
    {
      bool is_ear = true;
      //get adjacent indecies 
      int im1 = i == 0 ? n-1: i-1; // i-1 mod n
      int ip1 = (i+1)%n; // i+1 mod n

      vcl_pair<double,double> unit_vec_1(occl[ip1].first - occl[i].first,
        occl[ip1].second - occl[i].second);

      vcl_pair<double,double> unit_vec_2(occl[im1].first - occl[i].first,
        occl[im1].second - occl[i].second);


      double crossprod = ((occl[i].first-occl[im1].first) * (occl[ip1].second - occl[i].second)
        - (occl[i].second-occl[im1].second) * (occl[ip1].first - occl[i].first)) ;

      bool convex = crossprod * a > 0 ;

      if (n > 3)
      {
        if(!convex) 
        {
          is_ear = false;
          continue;
        }

        for(int j = 0; j < n; ++j)
        {
          //Compare triangle I with all points
          if( j == i || j == im1 || j == ip1 )
          {
            continue;
          }
          if(inside_triangle( occl[im1], occl[i], occl[ip1], occl[j]))
          {
            is_ear = false;
            break;
          }
        }
      }
      if(is_ear)
      {
        //Add ear to the triangle list
        occluder ear;
        ear.contour.push_back(occl[im1]);
        ear.contour.push_back(occl[i]);
        ear.contour.push_back(occl[ip1]);
        ear.frame_number = (*iter).frame_number;
        triangles.push_back(ear);
        //Remove point i from the contour
        occl.erase(occl.begin()+i);
        n-=1;
        i-=1;
        //added last triangle move to next contour
        if (n == 2)
        {
          break;
        }
      }
    }
  }
}

void mit_layer_annotation_reader::read_occulder_xml( TiXmlNode* pParent, 
                                                    vcl_vector<occluder>& occluders)
{
  if ( !pParent ) return;

  TiXmlNode* pChild;
  TiXmlText* pText;
  int t = pParent->Type();

  switch ( t )
  {
  case TiXmlNode::TINYXML_ELEMENT:
    // Frames and objects indicate that the last 
    // bounding box has been competely read in
    if (strcmp(pParent->Value(), "frame") == 0)
    {
      occluder topush;
      occluders.push_back(topush);
    }
    prev_element = pParent->Value();
    break;

  case TiXmlNode::TINYXML_TEXT:
    const char* pe;
    //The previous element indicates what this text node will be
    pe = prev_element.c_str();
    pText = pParent->ToText();

    if(strcmp(pe, "notes") == 0)
    {
      //This should be expanded
    }
    else if(strcmp(pe, "index") == 0)
    {
      occluders[occluders.size()-1].frame_number = atoi(pText->Value());
    }
    //Create a bounding box from the polygon in the xml file
    else if(strcmp(pe, "x") == 0)
    {
      vcl_pair<double, double> pt;
      pt.first = atof(pText->Value());
      occluders[occluders.size()-1].contour.push_back(pt);
    }
    else if(strcmp(pe, "y") == 0)
    {
      occluders[occluders.size()-1].contour[occluders[occluders.size()-1].contour.size()-1].second = atof(pText->Value());
    }
    break;

  default:
    break;
  }
  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
  {
    this->read_occulder_xml( pChild, occluders);
  }
}

bool lines_intersect(vcl_pair<double,double> ptA1, vcl_pair<double,double> ptA2,
                     vcl_pair<double,double> ptB1, vcl_pair<double,double> ptB2)
{
  double denom = ((ptB2.second - ptB1.second) * (ptA2.first - ptA1.first)) -
    ((ptB2.first - ptB1.first) * (ptA2.second - ptA1.second));
  if (denom == 0)
  {
    return false;
  }
  double ua = (((ptB2.first - ptB1.first) * (ptA1.second - ptB1.second)) - 
    ((ptB2.second - ptB1.second) * (ptA1.first - ptB1.first))) / denom;

  double ub = (((ptA2.first - ptA1.first) * (ptA1.second - ptB1.second)) - 
    ((ptA2.second - ptA1.second) * (ptA1.first - ptB1.first))) / denom;

  if((ua < 0) || (ua > 1) || (ub < 0) || (ub > 1))
  {
    return false;
  }
  return true;
}

bool is_occluded(double min_x, double min_y, double max_x, double max_y, double frame_index, vcl_vector<occluder>* occluders)
{
  bool occluded = false;
  vcl_pair<double, double> corner1(min_x,min_y);
  vcl_pair<double, double> corner2(max_x,min_y);
  vcl_pair<double, double> corner3(max_x,max_y);
  vcl_pair<double, double> corner4(min_x,max_y);
  vcl_vector<occluder>::iterator iter;


  //Check if any tracked point is in an occluder
  //or any occlder point is in a track the track
  //is occluded
  for(iter = occluders->begin(); iter != occluders->end(); ++iter)
  {
    //don't bother checking if it is not in the same frame
    if(frame_index != (*iter).frame_number)
    {
      continue;
    }
    //Check if any triangle points are in the bounding box
    vcl_vector< vcl_pair<double,double> >::iterator iter2;
    for(iter2 = (*iter).contour.begin(); iter2 != (*iter).contour.end(); ++iter2)
    {
      if((*iter2).first > min_x && (*iter2).first < max_x &&
        (*iter2).second > min_y && (*iter2).second < max_y)
      {
        occluded = true;
        break;
      }
    }
    //Check to see if any lines on the objects are intersecting
    if(!occluded)
    {
      for(int i = 0; i < 3 && !occluded; ++i)
      {
        occluded = 
          lines_intersect(corner1, corner2, (*iter).contour[i], (*iter).contour[(i+1)%3])||
          lines_intersect(corner2, corner3, (*iter).contour[i], (*iter).contour[(i+1)%3])||
          lines_intersect(corner3, corner4, (*iter).contour[i], (*iter).contour[(i+1)%3])||
          lines_intersect(corner4, corner1, (*iter).contour[i], (*iter).contour[(i+1)%3]);
      }
    }
    //Check if any bounding box points are in the triangle
    if(occluded || 
      inside_triangle(
      (*iter).contour[0],
      (*iter).contour[1],
      (*iter).contour[2],
      corner1) || 
      inside_triangle(
      (*iter).contour[0],
      (*iter).contour[1],
      (*iter).contour[2],
      corner2) || 
      inside_triangle(
      (*iter).contour[0],
      (*iter).contour[1],
      (*iter).contour[2],
      corner3) || 
      inside_triangle(
      (*iter).contour[0],
      (*iter).contour[1],
      (*iter).contour[2],
      corner4))
    {
      occluded = true;
      break;
    }

  }
  return occluded;
}

void 
mit_layer_annotation_reader
::read_xml( TiXmlNode* pParent, 
            vcl_vector<vidtk::track_sptr>& trks,
            vcl_vector< occluder >* occluders = 0)
{
  if ( !pParent ) return;

  TiXmlNode* pChild;
  TiXmlText* pText;
  int t = pParent->Type();

  switch ( t )
  {
  case TiXmlNode::TINYXML_ELEMENT:

    //Reading a new object push a new track and 
    //Reset any values that need reseting 
    if( strcmp(pParent->Value(), "object") == 0 )
    {
      vidtk::track_sptr topush = new vidtk::track;
      trks.push_back(topush);
    }
    prev_element = pParent->Value();
    break;

  case TiXmlNode::TINYXML_TEXT:
    const char* pe;
    //The previous element indicates what this text node will be
    pe = prev_element.c_str();
    pText = pParent->ToText();

    if(strcmp(pe, "id") == 0)
    {
      trks[trks.size()-1]->set_id( atoi( pText->Value() ) );
    }
    else if(strcmp(pe, "notes") == 0)
    {
      //This should be expanded
    }
    else if(strcmp(pe, "index") == 0)
    {
      frame_index = atoi(pText->Value());
    }
    //Create a bounding box from the polygon in the xml file
    else if(strcmp(pe, "x") == 0)
    {
      min_x = (min_x < atof(pText->Value()) && min_x != -1) ? min_x : atof(pText->Value());
      max_x = (max_x > atof(pText->Value()) && max_x != -1) ? max_x : atof(pText->Value());
	  //make sure there are no negative coordinates
	  min_x = min_x < 0 ? 0 : min_x;
	  max_x = max_x < 0 ? 0 : max_x;
    }
    else if(strcmp(pe, "y") == 0)
    {
      min_y = (min_y < atof(pText->Value()) && min_y != -1) ? min_y : atof(pText->Value());
      max_y = (max_y > atof(pText->Value()) && max_y != -1) ? max_y : atof(pText->Value());
	  //make sure there are no negative coordinates
	  min_y = min_y < 0 ? 0 : min_y;
	  max_y = max_y < 0 ? 0 : max_y;
    }
    break;

  default:
    break;
  }
  for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
  {
    read_xml( pChild, trks, occluders);
  }

  //Add a New Frame
  if (strcmp(pParent->Value(), "frame") == 0)
  {
    vnl_vector_fixed<double,3> loc;

    // Calculate the center of the bounding box
    loc[0] = (min_x+max_x)/2.0;
    loc[1] = (min_y+max_y)/2.0;
    loc[2] = 0;

    unsigned min[2];
    unsigned max[2];

    track_state_sptr to_add = new track_state; 
    to_add->loc_[0] = loc[0];
    to_add->loc_[1] = loc[1];
    to_add->loc_[2] = loc[2];
    to_add->time_.set_frame_number(frame_index+frame_indexing);
    if(frequency_of_frames != -1)
    {//time_ is a timestamp which stores time in microseconds
		//(frame number/frame frequency) is in seconds
		//so, convert from seconds to microseconds
      double t = ((frame_index+frame_indexing+1)/frequency_of_frames)*1e6;
      to_add->time_.set_time(t);
    }
    min[0] = (unsigned) min_x;
    min[1] = (unsigned) min_y;
    max[0] = (unsigned) max_x;
    max[1] = (unsigned) max_y;

    vgl_box_2d<unsigned> bbox = vgl_box_2d<unsigned>(min, max);

    image_object_sptr obj = new image_object;
    obj->img_loc_[0] = loc[0];
    obj->img_loc_[1] = loc[1];
    obj->bbox_.set_min_x(static_cast<unsigned>(min_x));
    obj->bbox_.set_min_y(static_cast<unsigned>(min_y));
    obj->bbox_.set_max_x(static_cast<unsigned>(max_x));
    obj->bbox_.set_max_y(static_cast<unsigned>(max_y));
    obj->area_ = (max_x-min_x)*(max_y-min_y);
    to_add->data_.set( tracking_keys::img_objs,
                            vcl_vector< image_object_sptr >( 1, obj ) );
    to_add->amhi_bbox_ = bbox;

    if(occluders == 0)
    {
      trks[trks.size()-1]->add_state(to_add);
    }
    else if(!is_occluded(min_x, min_y, max_x,max_y, frame_index, occluders))
    {
      trks[trks.size()-1]->add_state(to_add);
    }
    //Reset bbox temp variables
    min_x=-1;
    min_y=-1;
    max_x=-1;
    max_y=-1;
  }
}

bool mit_layer_annotation_reader::read(vcl_vector<track_sptr>& trks)
{
  vcl_vector<occluder> triangulated_occluders; 
  if(occluder_filename_ != "" && !ignore_occlusions_)
  {
    vcl_vector<occluder> occluders;
    TiXmlDocument oc_doc(occluder_filename_.c_str());
    if(oc_doc.LoadFile())
    {
      read_occulder_xml(&oc_doc,occluders);
    }
    triangulate(occluders,triangulated_occluders);
  }

  trks.clear();
  if(been_read_)
  {
    return false;
  }
  TiXmlDocument doc(filename_.c_str());
  if (doc.LoadFile())
  {
    if(ignore_occlusions_)
    {
      read_xml( &doc,trks,0);
    }
    else
    {
      read_xml( &doc,trks,&triangulated_occluders);
    }
  }
  else
  {
    vcl_cerr << "TinyXML couldn't load '" << filename_ << "'" << vcl_endl;
    return false;
  }
  been_read_ = true;
  return true;
}
