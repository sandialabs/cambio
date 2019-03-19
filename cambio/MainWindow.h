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


#ifndef MainWindow_H
#define MainWindow_H

#include <set>
#include <memory>
#include <vector>

#include <QUrl>
#include <QList>
#include <QRunnable>
#include <QMainWindow>

class QLabel;
class QLineEdit;
class QCheckBox;
class QDropEvent;
class QCloseEvent;
class QGridLayout;
class QToolButton;

class TimeView;
class SaveWidget;
class QHBoxLayout;
class SpectrumView;
class BusyIndicator;
class MeasurementInfo;
class FileDetailWidget;

class Measurement;

class MainWindow : public QMainWindow
{
  Q_OBJECT
  
public:
  explicit MainWindow( QWidget *parent = nullptr, Qt::WindowFlags flags = 0 );
  ~MainWindow();
  
  
public:
  bool eventFilter( QObject *object, QEvent *event );

  void handleSingleFileDrop( const QUrl &url );
  void handleMultipleFileDrop( const QList<QUrl> &urlist );
  
public slots:
  void setMeasurment( std::shared_ptr<MeasurementInfo> measurment );
  
  void recievedDropEvent( QDropEvent *event );
  
  void spectrumMouseLeft();
  void spectrumMousePositionChanged( int firstchannel, int lastchannel,
                                     double energy, double countheight,
                                     double channelcounts );
  
  void timeMouseLeft();
  void timeMousePositionChanged( int channel, double energy,
                                 double countheight,
                                 double gammacounts,
                                 double neutroncounts );
  void timeRangeSelected( int firstsample, int lastsample,
                          Qt::KeyboardModifiers mods );
  
  void refreshDisplays();
  void detectorsToDisplayChanged();
  void advanceSampleNumDisplayed();
  void decrementSampleNumDisplayed();
  void manualChangedSample();
  
  void createOpenFileDialog();
  void createAboutDialog();
  
  void toggleLogLinearYAxis();
  
  void gotoChartsTab();

  void closeEvent( QCloseEvent *event );
  
signals:
  void displayedSpectrumChanged( std::shared_ptr<MeasurementInfo>,
                                 std::set<int>,std::vector<bool> );
  
protected:
  //
  
  
  //For some reason the bellow are not called
  virtual void dragEnterEvent( QDragEnterEvent *event );
  virtual void dragMoveEvent( QDragMoveEvent *event );
  virtual void dragLeaveEvent( QDragLeaveEvent *event );
  virtual void dropEvent( QDropEvent *event );
  
  void displayMeasurment();
  void setTimeText( std::shared_ptr<const Measurement> meas );
  
  //calculateTimeSeriesData(...): calculates the time series histogram for
  //  passthrough/search-mode data.  The passed in Measurement shared ptr will
  //  be reset to point at a new histogram
  void calculateTimeSeriesData( std::shared_ptr<Measurement> &hist ) const;
  
  //updateForSampleNumChange(): sets stuff for m_currentSampleNum
  void updateForSampleNumChange();
  
  QTabWidget *m_tabs;

  SaveWidget *m_save;
  FileDetailWidget *m_detail;
  
  SpectrumView *m_spectrum;
  TimeView *m_time;
  QGridLayout *m_chartlayout;
  
  int m_currentSampleNum;
  QWidget *m_sampleChanger;
  QGridLayout *m_samplesLayout;
  QLineEdit *m_sampleEdit;
  QLabel *m_totalSamples;
  
  QWidget *m_detectors;
  QHBoxLayout *m_detectorsLayout;
  std::vector<QCheckBox *> m_detCheckBox;
  
  QToolButton *m_loglin;
  QLabel *m_livetime, *m_realtime, *m_deadtime;
  QLabel *m_mouseChannel, *m_mouseEnergy, *m_mouseHeight, *m_mouseChannelCounts;
  QWidget *m_statusFiller;
  
  
  std::set<int> m_displayedSampleNumbers;
  std::vector<bool> m_detectorsDisplayed;
  std::shared_ptr<MeasurementInfo> m_measurment;
};//class MainWindow

#endif //MainWindow
