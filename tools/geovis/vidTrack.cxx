/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vidTrack.h"
#include "vtkObjectFactory.h"
#include "vidTrackMetaData.h"

vtkStandardNewMacro(vidTrack);
vtkCxxRevisionMacro(vidTrack, "$Revision$");

// ----------------------------------------------------------------------------
vidTrack::vidTrack()
{
  this->StartFrame=0;
  this->LastFrame=0;
  this->Id=0;
  this->Boxes=new vtkstd::vector<vtkSmartPointer<vidBBox> >;
  this->MetaData=vidTrackMetaData::New();
}

// ----------------------------------------------------------------------------
vidTrack::~vidTrack()
{
  delete this->Boxes;
  this->MetaData->Delete();
}

// ----------------------------------------------------------------------------
vtkstd::vector<vtkSmartPointer<vidBBox> > *vidTrack::GetBoxes()
{
  return this->Boxes;
}

// ----------------------------------------------------------------------------
vidTrackMetaData *vidTrack::GetMetaData()
{
  return this->MetaData;
}

// ----------------------------------------------------------------------------
void vidTrack::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "StartFrame: " << this->StartFrame << endl;
  os << indent << "LastFrame: " << this->LastFrame << endl;
  os << indent << "Id: " << this->Id << endl;
  
  os << indent << "MetaData:" << endl;
  this->MetaData->PrintSelf(os,indent);
  
  size_t c=this->Boxes->size();
  
  size_t i=0;
  while(i<c)
    {
    os << indent << "BBox " << i <<":" <<endl;
    (*this->Boxes)[i]->PrintSelf(os,indent.GetNextIndent());
    ++i;
    }
}
