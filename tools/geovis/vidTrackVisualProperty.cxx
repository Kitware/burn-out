/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vidTrackVisualProperty.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vidTrackVisualProperty);
vtkCxxRevisionMacro(vidTrackVisualProperty, "$Revision$");

// ----------------------------------------------------------------------------
vidTrackVisualProperty::vidTrackVisualProperty()
{
  this->Visibility=true;
  this->BoxVisibility=true;
  this->TraceVisibility=true;
  this->Color[0]=1.0;
  this->Color[1]=1.0;
  this->Color[2]=1.0;
}

// ----------------------------------------------------------------------------
vidTrackVisualProperty::~vidTrackVisualProperty()
{
}

// ----------------------------------------------------------------------------
void vidTrackVisualProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Visibility: " << this->Visibility << endl;
  os << indent << "BoxVisibility: " << this->BoxVisibility << endl;
  os << indent << "TraceVisibility: " << this->TraceVisibility << endl;
  os << indent << "Color: " << this->Color[0] << "," << this->Color[1]
     << "," << this->Color[2] << endl; 
}
