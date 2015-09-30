/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vidVideoController - Synchronize the video reader and the texture.
// .SECTION Description
// 

#ifndef vidVideoController_h
#define vidVideoController_h

#include "vtkObject.h"
#include <QObject>

class vidVideoReader;
class vtkPixelBufferObject;
class vtkTextureObject;
class vtkRenderWindow;
class vtkTexture;
class vtkRenderer;
class vtkPlaneSource;
class vtkActor;
class vtkPolyDataMapper;
class vtkMultiThreader;
class vidMonitor;
class vtkCameraActor;
class vtkCamera;
class vidVideoMetaDataReader;
class vtkGeoSphereTransform;
class vtkPolyData;
class vidQueryResultsReader;
class vtkPropAssembly;

#include "vidTrackRepresentation.h"
#include "vidTrackVisualProperty.h"
#include "vtkSmartPointer.h"
#include <vtkstd/vector>


class vidVideoController : public QObject, public vtkObject
{
  Q_OBJECT
  
public:
  static vidVideoController *New();
  vtkTypeRevisionMacro(vidVideoController,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create the reader, PBO and TO.
  void Initialize(vtkRenderWindow *context);
  
  bool IsInitialized() const;
  
  // Description:
  // The gut of this class. Coordinate Reader and TO updates.
  // \pre is_initialized: IsInitialized()
  void SetFrame(int frameId);
  
  // Description:
  // Return the current frame.
  int GetFrame() const;
  
  vidVideoReader *GetReader() const;
  vidVideoMetaDataReader *GetMetaDataReader() const;
  vidQueryResultsReader *GetTracksReader() const;
  
  vtkTexture *GetTexture() const;
  
  // Name of the metainfo file (geolocalization). Initial value is "".
  // \post result_not_null: result!=0
  const vtkstd::string *GetMetaName() const;
  
  // Change name of the metainfo file.
  // \pre name_exists: name !=0
  // \post is_set: *GetMetaName()==*name
  void SetMetaName(const vtkstd::string *name);
  
  void SetRenderer(vtkRenderer *r);
  vtkRenderer *GetRenderer() const;
  
  // Description:
  // Default constructor: call Initialize right after.
  vidVideoController();
  
  ~vidVideoController();
  
  void SetIndex(size_t i);
  
  
  // Default is 500.
  double GetZHigh() const;
  void SetZHigh(double value);
  
  // Description:
  // The gut of this class. Coordinate Reader and TO updates.
  // \pre is_initialized: IsInitialized()
  // DON'T USE IT: called only by internal thread.
  void ReadFrame();
  
  // Description:
  // DON't USE IT: call by sub-thread when it is done with reading the frame.
  void ThreadDone();
  
  void SetMonitor(vidMonitor *m);
  
  void SetTraceVisibility(bool value);
  
  vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *
  GetTracksProperties() const;
  
  signals:
  void signalModelChanged();
  
public slots:
  void slotUpdateProperty(int p);
  void slotUpdateProperties();
  
protected:
  void AddSceneElements();
  void RemoveSceneElements();
  void ReadMetaData();
  void ReadTracks();
  
  vidVideoReader *Reader;
  vtkPixelBufferObject *PBO;
  vtkTextureObject *TO;
  vtkRenderWindow *Context;
  vtkTexture *Texture;
  vtkstd::string *MetaName;
  
  vtkRenderer *Renderer;
  vtkActor *VideoActor;
  vtkPolyDataMapper *VideoMapper;
  vtkPlaneSource *VideoPlaneSource;
  
  bool Initialized;
  size_t Index;
  
  vtkMultiThreader *Threader;
  int Thread;
  int FrameId;
  vidMonitor *Monitor;
  
  vtkCamera *Camera;
  vtkCameraActor *ViewActor;
  
  vidVideoMetaDataReader *MetaDataReader;
  
  vtkPolyData *Quad;
  vtkGeoSphereTransform *Transform;
  
  vtkPolyData *OpenBox;
  vtkPolyDataMapper *OpenBoxMapper;
  vtkActor *OpenBoxActor;
  
  vidQueryResultsReader *TracksReader;
  vtkPropAssembly *TracksActor;
  vtkTimeStamp TracksTime;
  
  double ZHigh; // hack
  
//  vtkActor *ConeActor;
//  vtkConeSource *ConeSource;
//  vtkPolyDataMapper *ConeMapper;
  
  vtkstd::vector<vtkSmartPointer<vidTrackRepresentation > > *TrackRepresentations;
  
  vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *TracksProperties;
  
  bool TraceVisibility;
  
};

#endif // #define vidVideoController_h
