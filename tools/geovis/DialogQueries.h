/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#ifndef DIALOG_QUERIES_H
#define DIALOG_QUERIES_H

#include <QDialog>
#include "ui_DialogQueries.h"

#include <vtkstd/vector>
#include "vtkSmartPointer.h"
#include "vidTrack.h"
#include "vidTrackVisualProperty.h"

class DialogQueries : public QDialog,
                      public Ui_Dialog
{
  Q_OBJECT
  
public:
  DialogQueries(QWidget *parent=0);
  
  void BuidList(
    vtkstd::vector<vtkSmartPointer<vidTrack > > *tracks,
    vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *properties);
  
  signals:
  void signalPropertyChanged(int row);
  void signalAllPropertiesChanged();
  
public slots:
  virtual void slotUpdateVisualProperties(QTableWidgetItem *item);
  virtual void slotUpdateVisualPropertiess(int row,int column);
  
protected:
  vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *Properties;
  
};
    
#endif // #ifndef DIALOG_QUERIES_H
