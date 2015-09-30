/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidVideoReader - read video frames and return them in vtkImageData
// .SECTION Description
// Latitudes and longitudes are expressed in degrees. Positive latitudes are
// north, positive longitudes are east.

#ifndef vidBBox_h
#define vidBBox_h

#include "vtkObject.h"

class vidBBox : public vtkObject
{
public:
  static vidBBox* New();
  vtkTypeRevisionMacro(vidBBox,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Id of the frame. Initial value is 0.
  vtkGetMacro(Frame,int);
  vtkSetMacro(Frame,int);
  
  // Description:
  // Abscissa of the upper left corner, expressed in pixel.
  // Initial value is 0.
//  vtkGetMacro(UpperLeftX,int);
//  vtkSetMacro(UpperLeftX,int);
  vtkGetMacro(Left,int);
  vtkSetMacro(Left,int);
  
  // Description:
  // Ordinate of the upper left corner, expressed in pixel.
  // Initial value is 0.
//  vtkGetMacro(UpperLeftY,int);
//  vtkSetMacro(UpperLeftY,int);
  vtkGetMacro(Upper,int);
  vtkSetMacro(Upper,int);
  
  // Description:
  // Abscissa of the lower right corner, expressed in pixel.
  // Initial value is 0.
//  vtkGetMacro(LowerRightX,int);
//  vtkSetMacro(LowerRightX,int);
  vtkGetMacro(Right,int);
  vtkSetMacro(Right,int);
 
  
  // Description:
  // Ordinate of the lower right corner, expressed in pixel.
  // Initial value is 0.
//  vtkGetMacro(LowerRightY,int);
//  vtkSetMacro(LowerRightY,int);
  vtkGetMacro(Lower,int);
  vtkSetMacro(Lower,int);
  
  
  // Description:
  // Time stamp. Initial value is 0.
  vtkGetMacro(TimeStamp,double);
  vtkSetMacro(TimeStamp,double);
  
protected:
  vidBBox();
  ~vidBBox();
  
  int Frame;
//  int UpperLeftX;
//  int UpperLeftY;
//  int LowerRightX;
//  int LowerRightY;
  int Left;
  int Right;
  int Upper;
  int Lower;
  double TimeStamp;
  
private:
  vidBBox(const vidBBox&); // Not implemented.
  void operator=(const vidBBox&); // Not implemented.
};

#endif
