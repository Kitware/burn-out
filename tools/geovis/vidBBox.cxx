/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vidBBox.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vidBBox);
vtkCxxRevisionMacro(vidBBox, "$Revision$");

// ----------------------------------------------------------------------------
vidBBox::vidBBox()
{
  this->Frame=0;
  this->Left=0;
  this->Right=0;
  this->Upper=0;
  this->Lower=0;
  this->TimeStamp=0;
}

// ----------------------------------------------------------------------------
vidBBox::~vidBBox()
{
}

// ----------------------------------------------------------------------------
void vidBBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "Frame: " << this->Frame << endl;
  os << indent << "Left: " << this->Left << endl;
  os << indent << "Right: " << this->Right << endl;
  os << indent << "Upper: " << this->Upper << endl;
  os << indent << "Lower: " << this->Lower << endl;
  os << indent << "TimeStamp: " << this->TimeStamp << endl;
}
