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


#ifndef SaveWidget_H
#define SaveWidget_H

#include <set>
#include <memory>
#include <vector>

#include <QWidget>


class QLabel;
class QDialog;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QGridLayout;
class QListWidget;
class QButtonGroup;

class MeasurementInfo;
class BatchConvertDialog;

class SaveWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit SaveWidget( QWidget *parent = 0, Qt::WindowFlags f = nullptr );
  ~SaveWidget();

  void initBatchConvertion( QString directory );
  void initBatchConvertion( QList<QString> files );

public slots:
  void updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                      std::set<int> samplenums,
                      std::vector<bool> detectors );

  void save();
  void browseForDirectory();
  void formatChanged();
  void checkValidDirectory();
  
  /** Returns the currently selected save path.
   Returns empty string on invalid path.
   */
  QString saveDirectory();
  
protected:
  QGridLayout *m_layout;
  QLineEdit *m_saveDirectory;
  QLineEdit *m_saveName;
  QListWidget *m_saveAsType;
  QLabel *m_formatDescription;
  QLabel *m_noOptionsText;
  
  //WhatToSaveInSingleSpecFile: Save options, when saving to a file format that
  //  supports only a single spectrum/record in the file.  This enum cooresponds
  //  to button id's in m_saveToSingleSpecFileGroup.
  enum WhatToSaveInSingleSpecFile
  {
    //When we are displaying a single record from the input file (which has
    //  multiple records, but is not a search-mode/passthrough, or multiple
    //  detector setup), and that is the only record we will save into the
    //  output file.
    kCurrentlyDisplayedOnly,
    
    //When there are multiple records in the input file, and the spectrum chart
    //  is currently showing a sum of multiple of them (either because it is
    //  search-mode/passthrough, or there are multiple detectors per sample),
    //  and its this summed spectrum we will save in the output file.
    kShowingSpectraSummedToSingle,
    
    //When there are multiple records in the input file (for any reason), and
    //  we will save the sum of all of them as the output spectrum.
    kAllSpectraSummedToSingle,
    
    //When there are multiple records in the input file (for any reason), and we
    //  will save each record as its own output file.
    kEachSpectraInSeperateFile,
    
    //To help us do things in loops (book keeping)
    NumWhatToSaveInSingleSpecFile
  };//enum WhatToSaveInSingleSpecFile
  
  QButtonGroup *m_saveToSingleSpecFileGroup;
  QGroupBox *m_saveToSingleSpecFile;
  
  //WhatToSaveInMultiSpecFile: Save options when saving to a file format that
  //  supports saving multiple spectra/records to the file.  This enum
  //  cooresponds to button id's in m_saveToMultiSpecFileGroup.
  enum WhatToSaveInMultiSpecFile
  {
    //When we want to save all the records in the input file to the output file
    //  without summing them or altering them if possible.  Each record in the
    //  input file will coorespond to a record in the output file.
    kAllSpecSeperatelyToSingleFile,
    
    //When we are showing a single record from the input file, and this is the
    //  only record we will write to the output file.  Note that this doesnt
    //  apply search-mode/passthrough files, or systems with mutliple detectors.
    kCurrentSingleSpecOnly,
    
    //When the energy chart is the result of multiple records summed together,
    //  we will save this summed spectra as a single record in the output file.
    //  This option applies to any input file with multiple records.
    kShowingSpectraAsSingle,
    
    //When we want to save the sum of all records in the input file to a single
    //  record in the output file.  Applies to any input file with multiple
    //  records.
    kAllSpecSummedToSingle,
    
    //When we want to save each record in the input file, to a seperate output
    //  file.  Applies to any input file with multiple records.
    kEachSpecToSeperateFile,
    
    //When we want to sum all detectors for each sample number, resulting in one
    //  record for each sample number (so if there were 8 gamma detectors, this
    //  would typically result in there being 8 times fewer output records than
    //  input records).  Applies to input files that have multiple gamma
    //  detectors whos sampling cooresponds to the same time intervals; typical
    //  of RPMs and search systems.
    kSumDetectorsForeachSampleNumber,
    
    //To help us do things in loops (book keeping)
    NumWhatToSaveInMultiSpecFile
  };//enum WhatToSaveInMultiSpecFile
  
  QButtonGroup *m_saveToMultiSpecFileGroup;
  QGroupBox *m_saveToMultiSpecFile;
  
  QPushButton *m_saveButton;
  
  std::set<int> m_samplenums;
  std::vector<bool> m_detectors;
  std::shared_ptr<MeasurementInfo> m_measurment;
};//class SaveWidget

#endif
