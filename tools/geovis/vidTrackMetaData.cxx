/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vidTrackMetaData.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vidTrackMetaData);
vtkCxxRevisionMacro(vidTrackMetaData, "$Revision$");

// ----------------------------------------------------------------------------
vidTrackMetaData::vidTrackMetaData()
{
  this->TimeStamp=0.0;
  this->SensorLatitude=0.0;
  this->SensorLongitude=0.0;
  this->UpperLeftLatitude=0.0;
  this->UpperLeftLongitude=0.0;
  this->UpperRightLatitude=0.0;
  this->UpperRightLongitude=0.0;
  this->LowerLeftLatitude=0.0;
  this->LowerLeftLongitude=0.0;
  this->LowerRightLatitude=0.0;
  this->LowerRightLongitude=0.0;
  this->HorizontalFieldOfView=0.0;
  this->VerticalFieldOfView=0.0;
  this->SlantRange=0.0;
}

// ----------------------------------------------------------------------------
vidTrackMetaData::~vidTrackMetaData()
{
}

// ----------------------------------------------------------------------------
void vidTrackMetaData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "TimeStamp: " << this->TimeStamp << endl;
  os << indent << "SensorLatitude: " << this->SensorLatitude << endl;
  os << indent << "SensorLongitude: " << this->SensorLongitude << endl;
  
  os << indent << "UpperLeftLatitude: "
     << this->UpperLeftLatitude << endl;
  os << indent << "UpperLeftLongitude: "
     << this->UpperLeftLongitude << endl;
  os << indent << "UpperRightLatitude: "
     << this->UpperRightLatitude << endl;
  os << indent << "UpperRightLongitude: "
     << this->UpperRightLongitude << endl;
  os << indent << "LowerLeftLatitude: "
     << this->LowerLeftLatitude << endl;
  os << indent << "LowerLeftLongitude: "
     << this->LowerLeftLongitude << endl;
  os << indent << "LowerRightLatitude: "
     << this->LowerRightLatitude << endl;
  os << indent << "LowerRightLongitude: "
     << this->LowerRightLongitude << endl;
  os << indent << "HorizontalFieldOfView: "
     << this->HorizontalFieldOfView << endl;
  os << indent << "VerticalFieldOfView: "
     << this->VerticalFieldOfView << endl;
  os << indent << "SlantRange: "
     << this->SlantRange << endl;
}
