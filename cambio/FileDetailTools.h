/**
 Cambio: a simple program to convert or manipulate gamma spectrum data files.
 Copyright (C) 2015 William Johnson
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef FileDetailTools_H
#define FileDetailTools_H

#include <set>
#include <memory>
#include <vector>

#include <QDialog>


class QLabel;
class QSpinBox;
class QLineEdit;
class QCloseEvent;
class QPushButton;
class QButtonGroup;
class QDoubleSpinBox;

class MeasurementInfo;
class FileDetailWidget;


//This file implements the dialogs that are triggered by the user clicking on
//  one of the buttons on the "Details" tab.

class CombineChannelsDialog : public QDialog
{
  Q_OBJECT
  
public:
  CombineChannelsDialog( FileDetailWidget *parent );
  virtual ~CombineChannelsDialog();
  
public slots:
  void doChanges();
  void checkNumChannelsValidity();
  
protected:
  virtual void closeEvent( QCloseEvent *event );
  
  int m_nchannel;
  QLabel *m_status;
  QLineEdit *m_ncombine;
  QButtonGroup *m_applyToGroup;
  QPushButton *m_accept;
  FileDetailWidget *m_parent;
};//class CombineChannelsDialog



class TruncateChannelsDialog : public QDialog
{
  Q_OBJECT
  
public:
  TruncateChannelsDialog( FileDetailWidget *parent );
  virtual ~TruncateChannelsDialog();
  
  public slots:
  void doChanges();
  void energyRangeChanged();
  void channelRangeChanged();
  
protected:
  virtual void closeEvent( QCloseEvent *event );
  
  QSpinBox *m_lowerChannel;
  QSpinBox *m_upperChannel;
  
  QDoubleSpinBox *m_lowerEnergy;
  QDoubleSpinBox *m_upperEnergy;
  
  QButtonGroup *m_applyToGroup;
  
  QPushButton *m_accept;
  
  FileDetailWidget *m_parent;
};//class CombineChannelsDialog


class EnergyCalDialog : public QDialog
{
  Q_OBJECT
  
public:
  EnergyCalDialog( FileDetailWidget *parent );
  virtual ~EnergyCalDialog();
  
public slots:
  void removeCal();
  
  void updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                                      std::set<int> samplenums,
                                      std::vector<bool> detectors );

protected:
  virtual void closeEvent( QCloseEvent *event );
  
  std::shared_ptr<MeasurementInfo> m_original;
  
  QPushButton *m_remove_cal;
  
  FileDetailWidget *m_parent;
};//class EnergyCalDialog


#endif  //FileDetailTools_H
