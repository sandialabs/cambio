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


#ifndef FileDetailWidget_H
#define FileDetailWidget_H

#include <set>
#include <memory>
#include <vector>

#include <QWidget>


class QLabel;
class QSpinBox;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QDateTimeEdit;

class Measurement;
class MeasurementInfo;
class EnergyCalDialog;
class CombineChannelsDialog;
class TruncateChannelsDialog;


class FileDetailWidget : public QWidget
{
  Q_OBJECT
  
public:
  explicit FileDetailWidget( QWidget *parent = nullptr, Qt::WindowFlags f = 0 );
  ~FileDetailWidget();
  
public slots:
  void updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                      std::set<int> samplenums, std::vector<bool> detectors );
  void changeRecord( int record );
  void specTitleChanged();
  void specCommentsChanged();
  void fileCommentsChanged();
  void filenameChanged();
  void uuidChanged();
  void inspectionChanged();
  void laneChanged();
  void locationChanged();
  void instTypeChanged();
  void manufacturerChanged();
  void modelChanged();
  void serialChanged();
  void realtimeChanged();
  void livetimeChanged();
  void latitudeChanged();
  void longitudeChanged();
  void dateTimeChanged();
  void posDateTimeChanged();
  
  void createCombineChannelDialog();
  void combineChannels( const int ncombine, const bool wholefile );
  
  void createCropChannelDialog();
  void cropToChannels( int first_channel, int last_channel, const bool all );

  void createEnergyCalDialog();
  void energyCalUpdated( std::shared_ptr<MeasurementInfo> meas );

signals:
  void fileDataModified();
  void energyCalDialogCreated();
  
  ///Emited from inside updateDisplay(...) to propogate to energy cal dialog
  void displayUpdated( std::shared_ptr<MeasurementInfo> meas,
                      std::set<int> samplenums, std::vector<bool> detectors );
protected:
  QLineEdit *m_filename, *m_uuid, *m_inspection, *m_lane, *m_location;
  QLineEdit *m_instType, *m_manufacturer, *m_model, *m_serial;
  QLabel *m_memsize;
  QTextEdit *m_fileComments;
  
  QDateTimeEdit *m_date, *m_posdate;
  QLineEdit *m_realtime, *m_livetime, *m_latitude, *m_longitude;
  QLineEdit *m_specTitle;
  QTextEdit *m_specComments;
  QLabel *m_neutroncount, *m_gammacount, *m_numchannels, *m_energy;
  
  QSpinBox *m_record;
  QLabel *m_numrecords;
  
  std::set<int> m_samplenums;
  std::vector<bool> m_detectors;
  
  std::shared_ptr<const Measurement> m_meas;
  std::shared_ptr<MeasurementInfo> m_measurment;
  
  friend class EnergyCalDialog;
  friend class CombineChannelsDialog;
  friend class TruncateChannelsDialog;
};//class FileDetailWidget

#endif
