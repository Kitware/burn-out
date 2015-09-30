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
// 

#ifndef vidVideoMetaDataReader_h
#define vidVideoMetaDataReader_h

#include "vtkObject.h"
#include "vtkSmartPointer.h"
#include <vtkstd/vector>

class vidStep;
class vtkTimeStamp;

class vidVideoMetaDataReader : public vtkObject
{
public:
  static vidVideoMetaDataReader* New();
  vtkTypeRevisionMacro(vidVideoMetaDataReader,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Name of the meta video file.
  void SetFileName(const vtkstd::string *fileName);
  const vtkstd::string *GetFileName() const;
  
   // Description:
  // Name of the meta file with the homogeneous 4x4 matrices.
  void SetMatrixFileName(const vtkstd::string *matrixFileName);
  const vtkstd::string *GetMatrixFileName() const;
  
  
  void SetFrame(int frameId);
  int GetFrame() const;
  
  // Description:
  // Read the current frame.
  // Change the value of LastFrameValid()
  void ReadFrame();
  
  // Description:
  // Tells if the last read was successful.
  bool LastFrameValid() const;
  
  // Description:
  // \pre valid: LastFrameValid()
  vidStep *GetLastFrame() const;
  
protected:
  vidVideoMetaDataReader();
  ~vidVideoMetaDataReader();
  
  bool ReadMetaDataFile();
  
  vtkstd::string *FileName;
  vtkstd::string *MatrixFileName;
  
  int Frame;

  vtkstd::vector<vtkSmartPointer<vidStep> > *Steps;
  
  vtkTimeStamp MetaFileReadTime;
  vidStep *LastFrame;
  
private:
  vidVideoMetaDataReader(const vidVideoMetaDataReader&); // Not implemented.
  void operator=(const vidVideoMetaDataReader&); // Not implemented.
};

#endif
