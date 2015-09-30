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

#ifndef vidVideoReader_h
#define vidVideoReader_h

#include "vtkImageAlgorithm.h"

namespace vidtk
{
class vidl_ffmpeg_frame_process;
};

class vtkImageImport;
class vtkImageFlip;
class vtkImageAppendComponents;

class vidVideoReader : public vtkImageAlgorithm
{
public:
  static vidVideoReader* New();
  vtkTypeRevisionMacro(vidVideoReader,vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Name of video file.
  void SetFileName(const vtkstd::string *fileName);
  const vtkstd::string *GetFileName() const;
  
  void SetFrame(int frameId);
  int GetFrame() const;
protected:
  vidVideoReader();
  ~vidVideoReader();
  
  int RequestInformation(vtkInformation *request,
                         vtkInformationVector **inputVector,
                         vtkInformationVector *outputVector);
  
  int RequestData(vtkInformation *request,
                  vtkInformationVector **inputVector,
                  vtkInformationVector *outputVector);
  
  int ReadFrame();
  
  vtkstd::string *FileName;
  int Frame;
  
  int LastFrame;
  bool Opened;
  
  vidtk::vidl_ffmpeg_frame_process *Decoder;
  vtkImageImport *Importer;
  vtkImageFlip *Flipper;
  
  vtkImageImport *ImporterGreen;
  vtkImageImport *ImporterBlue;
  vtkImageAppendComponents *Components;
  
private:
  vidVideoReader(const vidVideoReader&); // Not implemented.
  void operator=(const vidVideoReader&); // Not implemented.
};

#endif
