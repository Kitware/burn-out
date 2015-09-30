/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vidStep.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vidStep);
vtkCxxRevisionMacro(vidStep, "$Revision$");

// ----------------------------------------------------------------------------
vidStep::vidStep()
{
  this->FrameInShot=0;
  this->FrameInSource=0;
  this->TimeStamp=0.0;
  this->Valid=false;
  this->SensorLatitude=0.0;
  this->SensorLongitude=0.0;
  this->SensorAltitude=0.0;
  this->FrameUpperLeftLatitude=0.0;
  this->FrameUpperLeftLongitude=0.0;
  this->FrameUpperRightLatitude=0.0;
  this->FrameUpperRightLongitude=0.0;
  this->FrameLowerLeftLatitude=0.0;
  this->FrameLowerLeftLongitude=0.0;
  this->FrameLowerRightLatitude=0.0;
  this->FrameLowerRightLongitude=0.0;
}

// ----------------------------------------------------------------------------
vidStep::~vidStep()
{
}

// ----------------------------------------------------------------------------
void vidStep::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "FrameInShot: " << this->FrameInShot << endl;
  os << indent << "FrameInSource: " << this->FrameInSource << endl;
  os << indent << "TimeStamp: " << this->TimeStamp << endl;
  os << indent << "Valid: " << this->Valid << endl;
  os << indent << "SensorLatitude: " << this->SensorLatitude << endl;
  os << indent << "SensorLongitude: " << this->SensorLongitude << endl;
  os << indent << "SensorAltitude: " << this->SensorAltitude << endl;
  os << indent << "FrameUpperLeftLatitude: "
     << this->FrameUpperLeftLatitude << endl;
  os << indent << "FrameUpperLeftLongitude: "
     << this->FrameUpperLeftLongitude << endl;
  os << indent << "FrameUpperRightLatitude: "
     << this->FrameUpperRightLatitude << endl;
  os << indent << "FrameUpperRightLongitude: "
     << this->FrameUpperRightLongitude << endl;
  os << indent << "FrameLowerLeftLatitude: "
     << this->FrameLowerLeftLatitude << endl;
  os << indent << "FrameLowerLeftLongitude: "
     << this->FrameLowerLeftLongitude << endl;
  os << indent << "FrameLowerRightLatitude: "
     << this->FrameLowerRightLatitude << endl;
  os << indent << "FrameLowerRightLongitude: "
     << this->FrameLowerRightLongitude << endl;
}
