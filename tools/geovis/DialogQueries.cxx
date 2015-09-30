/*=========================================================================

  Program:   vidtk

  Copyright (c) Kitware, Inc.
  All rights reserved.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "DialogQueries.h"
#include <cassert>

// ----------------------------------------------------------------------------
DialogQueries::DialogQueries(QWidget *parent)
  : QDialog(parent)
{
  setupUi(this);
 
  this->Properties=0;
}

// ----------------------------------------------------------------------------
void DialogQueries::BuidList(
  vtkstd::vector<vtkSmartPointer<vidTrack > > *tracks,
  vtkstd::vector<vtkSmartPointer<vidTrackVisualProperty > > *properties)
{
  QTableWidget *t=this->tableWidget;
  
  size_t c;
  size_t i;
  c=tracks->size();
  
  t->setColumnCount(3);
  // +1 for checkall row, as we cannot add check box to headers.
  t->setRowCount(c+1);
  t->setHorizontalHeaderLabels(QStringList()<<tr("track")<<tr("box")<<tr("trace"));
  t->verticalHeader()->setVisible(false);
  
  bool allTracks=true;
  bool allBoxes=true;
  bool allTraces=true;
  
  i=0;
  while(i<c)
    {
    vidTrack *track=(*tracks)[i];
    vidTrackVisualProperty *p=(*properties)[i];
    QTableWidgetItem *item0=new QTableWidgetItem(QString::number(i));
    item0->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    Qt::CheckState s;
    
    allTracks=allTracks && p->GetVisibility();
    allBoxes=allBoxes && p->GetBoxVisibility();
    allTraces=allTraces && p->GetTraceVisibility();
    
    if(p->GetVisibility())
      {
      s=Qt::Checked;
      }
    else
      {
      s=Qt::Unchecked;
      }
    item0->setCheckState(s);
    
    
    double red;
    double green;
    double blue;
    p->GetColor(red,green,blue);
    
    QBrush brush;
    QColor color(red*255.0,green*255.0,blue*255.0);
//    QColor color(255,0,0);
    brush.setColor(color);
    
    brush.setStyle(Qt::SolidPattern);
    
    item0->setBackground(brush);
    
    t->setItem(i+1,0,item0);
    
    QTableWidgetItem *item1=new QTableWidgetItem();
    item1->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    if(p->GetBoxVisibility())
      {
      s=Qt::Checked;
      }
    else
      {
      s=Qt::Unchecked;
      }
    item1->setCheckState(s);
    t->setItem(i+1,1,item1);
    
    QTableWidgetItem *item2=new QTableWidgetItem();
    item2->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
    if(p->GetTraceVisibility())
      {
      s=Qt::Checked;
      }
    else
      {
      s=Qt::Unchecked;
      }
    item2->setCheckState(s);
    t->setItem(i+1,2,item2);
    
    ++i;
    }
                                                 
  QTableWidgetItem *header0=new QTableWidgetItem(QString("all tracks"));
  header0->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  Qt::CheckState s;
  if(allTracks)
    {
    s=Qt::Checked;
    }
  else
    {
    s=Qt::Unchecked;
    }
  header0->setCheckState(s);
  t->setItem(0,0,header0);
              
  QTableWidgetItem *header1=new QTableWidgetItem(QString("all boxes"));
  header1->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  if(allBoxes)
    {
    s=Qt::Checked;
    }
  else
    {
    s=Qt::Unchecked;
    }
  header1->setCheckState(s);
  t->setItem(0,1,header1);
  
  QTableWidgetItem *header2=new QTableWidgetItem(QString("all traces"));
  header2->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
  if(allTraces)
    {
    s=Qt::Checked;
    }
  else
    {
    s=Qt::Unchecked;
    }
  header2->setCheckState(s);
  t->setItem(0,2,header2);
  
//  connect(this->ui->actionExit, SIGNAL(triggered()), this, SLOT(slotExit()));
//  connect(t,SIGNAL(itemClicked(QTableWidgetItem *)),this,SLOT(slotUpdateVisualProperties(QTableWidgetItem *)));
  connect(t,SIGNAL(cellClicked(int,int)),this,SLOT(slotUpdateVisualPropertiess(int,int)));
  
  this->Properties=properties;
}

// ----------------------------------------------------------------------------
void DialogQueries::slotUpdateVisualProperties(QTableWidgetItem *item)
{
  std::cout << "some item clicked." << std::endl;
  
}
// ----------------------------------------------------------------------------
void DialogQueries::slotUpdateVisualPropertiess(int row,int column)
{
  std::cout << "some cell clicked. row=" << row<< " column" << column << std::endl;
  
  QTableWidget *t=this->tableWidget;
  QTableWidgetItem *i=t->item(row,column);
  bool status=i->checkState()==Qt::Checked;
  
  size_t c;
  size_t index;
  
  
  if(row==0) // check all
    {
     c=this->Properties->size();
     index=0;
     while(index<c)
       {
       vidTrackVisualProperty *p=(*this->Properties)[index];
       switch(column)
         {
         case 0:
           p->SetVisibility(status); // update model
           break;
         case 1:
           p->SetBoxVisibility(status); // update model
           break;
         case 2:
           p->SetTraceVisibility(status); // update model
           break;
         default:
           assert("check: impossible case" && 0);
           break; 
         }
       // update view
       QTableWidgetItem *item2=t->item(index+1,column);
       item2->setCheckState(i->checkState());
       ++index;
       }
     
    emit signalAllPropertiesChanged();
    }
  else
    {
    vidTrackVisualProperty *p=(*this->Properties)[row-1];
    
    switch(column)
      {
      case 0:
        p->SetVisibility(status);
        break;
      case 1:
        p->SetBoxVisibility(status);
        break;
      case 2:
        p->SetTraceVisibility(status);
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    
    // Update first "all check" row
    c=this->Properties->size();
    i=0;
    
    bool all=true;
    index=0;
    switch(column)
      {
      case 0:
        while(all && index<c)
          {
          p=(*this->Properties)[index];
          all=all && p->GetVisibility();
          ++index;
          }
        i=t->item(0,0);
        break;
      case 1:
        while(all && index<c)
          {
          p=(*this->Properties)[index];
          all=all && p->GetBoxVisibility();
          ++index;
          }
        i=t->item(0,1);
        break;
      case 2:
        while(all && index<c)
          {
          p=(*this->Properties)[index];
          all=all && p->GetTraceVisibility();
          ++index;
          }
        i=t->item(0,2);
        break;
      default:
        assert("check: impossible case" && 0);
        break;
      }
    Qt::CheckState s;
    if(all)
      {
      s=Qt::Checked;
      }
    else
      {
      s=Qt::Unchecked;
      }
    i->setCheckState(s);
    
    emit signalPropertyChanged(row-1);
    }
}
