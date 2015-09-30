/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef _VIRATGeovis_h
#define _VIRATGeovis_h

#include <vtkSmartPointer.h>    // Required for smart pointer internal ivars.

#include <QMainWindow>

class Ui_VIRATGeovis;
class vtkGeoAlignedImageSource;
class vtkGeoGlobeSource;
class vtkGeoView;
class vidMapView;
class vidVideoController;
class vtkAnimationScene;

class QComboBox;
class vidRichedVideo;
class vidMonitor;
class DialogQueries;
class vtkMinimalStandardRandomSequence;

class VIRATGeovis : public QMainWindow
{
  Q_OBJECT

public:
  VIRATGeovis(); 
  ~VIRATGeovis();

  virtual void loadImage(const char *fileName);
  virtual void loadMapImage(const char *fileName);
  virtual void AddVideoFileName(const vtkstd::string *fileName);
  virtual void AddVideoAndMetaFileName(const vtkstd::string *fileName,
                                       const vtkstd::string *metaFileName);
  virtual void AddVideoAndQueryResultFileName(
    const vtkstd::string *fileName,
    const vtkstd::string *queryResultFileName);
public slots:
  virtual void slotOpenQueryResults();
  virtual void slotExit();
  virtual void slotPlay();
  virtual void slotPause();
  virtual void slotSnap();
  virtual void slotActiveVideo(int);
  virtual void slotCheckTraces();
  virtual void slotDialogQueries();
  virtual void slotUpdateView();
  
protected:
  void UpdateComboBox();
  
  vtkSmartPointer<vidMapView> MapView;
  vtkSmartPointer<vtkGeoView> GeoView;
  vtkSmartPointer<vtkGeoGlobeSource> TerrainSource;
  vtkSmartPointer<vtkGeoAlignedImageSource> ImageSource;
  
  // Designer form
  Ui_VIRATGeovis *ui;
  
  vtkAnimationScene *AnimationScene;
  
  QComboBox *ComboBox;
  
  std::vector<vtkSmartPointer<vidVideoController > > *Videos;
  int ActiveVideo;
  vidMonitor *Monitor;
  bool CheckTraces;
  DialogQueries *DiaQueries;
  vtkMinimalStandardRandomSequence *Randomizer;
  
//protected slots:
};

#endif // _VIRATGeovis_h
