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

#ifndef vidQueryResultsReader_h
#define vidQueryResultsReader_h

#include "vtkObject.h"
#include <vtkstd/vector>
#include "vtkSmartPointer.h"
#include "vidTrack.h"
#include "vidBBox.h"
#include "vtk_libxml2.h" // for xmlNode type
#include VTKLIBXML2_HEADER(parser.h)
#include VTKLIBXML2_HEADER(tree.h)

class vidQueryResultsReader : public vtkObject
{
public:
  static vidQueryResultsReader* New();
  vtkTypeRevisionMacro(vidQueryResultsReader,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Name of video file.
  void SetFileName(const vtkstd::string *fileName);
  const vtkstd::string *GetFileName() const;
  
  // void SetFrame(int frameId);
//  int GetFrame() const;
  
  bool ReadXMLFile();
  
  // Description:
  // List of tracks
  vtkstd::vector<vtkSmartPointer<vidTrack > > *GetTracks() const;
  
protected:
  vidQueryResultsReader();
  ~vidQueryResultsReader();
  
  bool ParseRoot(xmlNode *root);
  bool ParseTracks(xmlNode *firstChild);
  vidTrack *ParseTrack(xmlNode *track);
  bool ParseTrackChildren(xmlNode *firstChild,
                          vidTrack *track);
  bool ParseSourceTrackIDs(xmlNode *node);
  bool ParseTrackStyle(xmlNode *node);
  vidBBox *ParseBBox(xmlNode *node);
  bool ParseDescriptor(vidTrack *track,
                       xmlNode *node);
  bool ParseMetadataDescriptor(vidTrack *track,
                               xmlNode *firstChild);
  
  vtkstd::string *FileName;
//  int Frame;
  
//  int LastFrame;
//  bool Opened;
  
  vtkstd::vector<vtkSmartPointer<vidTrack > > *Tracks;
  
  vtkTimeStamp ReadTime;
  
private:
  vidQueryResultsReader(const vidQueryResultsReader&); // Not implemented.
  void operator=(const vidQueryResultsReader&); // Not implemented.
};

#endif
