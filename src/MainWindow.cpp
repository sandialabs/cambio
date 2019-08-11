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


#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

#include <QIcon>
#include <QDebug>
#include <QLabel>
#include <QPixmap>
#include <QMenuBar>
#include <QSettings>
#include <QFileInfo>
#include <QLineEdit>
#include <QCheckBox>
#include <QMimeData>
#include <QTabWidget>
#include <QDropEvent>
#include <QStatusBar>
#include <QGridLayout>
#include <QCloseEvent>
#include <QGridLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QFileDialog>
#include <QThreadPool>
#include <QApplication>
#include <QIntValidator>
#include <QSignalMapper>
#include <QDesktopWidget>
#include <QDragLeaveEvent>
#include <QDragEnterEvent>
#include <QCoreApplication>

#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "cambio/TimeView.h"
#include "cambio/SaveWidget.h"
#include "cambio/MainWindow.h"
#include "cambio/SpectrumView.h"
#include "cambio/BusyIndicator.h"
#include "cambio/SpectrumChart.h"
#include "cambio/FileDetailWidget.h"

#include "cambio/left_arrow.hpp"
#include "cambio/right_arrow.hpp"
#include "cambio/about_icons.hpp"

#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"

#if defined(__APPLE__)
#include "cambio/macos_helper.h"
#endif


using namespace std;

#if( !defined(QT_CHARTS_VERSION) || QT_CHARTS_VERSION < 0x020000 )
QTCOMMERCIALCHART_USE_NAMESPACE
#else
QT_CHARTS_USE_NAMESPACE
#endif

MainWindow::MainWindow( QWidget *parent, Qt::WindowFlags flags )
  : QMainWindow( parent, flags ),
    m_tabs( new QTabWidget ),
    m_save( new SaveWidget ),
    m_detail( new FileDetailWidget ),
    m_spectrum( new SpectrumView() ),
    m_time( new TimeView() ),
    m_chartlayout( new QGridLayout() ),
    m_currentSampleNum( -1 ),
    m_sampleChanger( new QWidget() ),
    m_samplesLayout( new QGridLayout() ),
    m_sampleEdit( new QLineEdit() ),
    m_totalSamples( new QLabel() ),
    m_detectors( new QWidget() ),
    m_detectorsLayout( new QGridLayout() ),
    m_loglin( new QToolButton ),
    m_livetime( new QLabel ),
    m_realtime( new QLabel ),
    m_deadtime( new QLabel ),
    m_mouseChannel( new QLabel ),
    m_mouseEnergy( new QLabel ),
    m_mouseHeight( new QLabel ),
    m_mouseChannelCounts( new QLabel ),
    m_statusFiller( new QWidget() )
{
  setAcceptDrops( true );
  QCoreApplication::setOrganizationDomain( "gov" );
  QCoreApplication::setOrganizationName( "sandia" );
  QCoreApplication::setApplicationName( "cambio" );
  QCoreApplication::setApplicationVersion( "2.1.1" );

  
  setWindowTitle( tr("Cambio") );
  setUnifiedTitleAndToolBarOnMac( true );
  
  QWidget *holder = new QWidget();
  m_chartlayout->addWidget( m_spectrum, 0, 0 );
  m_chartlayout->addWidget( m_time, 1, 0 );
  m_chartlayout->addWidget( m_sampleChanger, 2, 0 );
  m_chartlayout->addWidget( m_detectors, 3, 0 );
  m_chartlayout->setRowStretch( 3, 1 );
  
  
  holder->setLayout( m_chartlayout );
  m_chartlayout->setRowStretch( 0, 4 );
  m_chartlayout->setRowStretch( 1, 2 );
  m_chartlayout->setRowStretch( 2, 0 );
  m_chartlayout->setRowStretch( 3, 0 );
  m_chartlayout->setSpacing( 0 );
  m_chartlayout->setVerticalSpacing( 0 );
  m_chartlayout->setHorizontalSpacing( 0 );
  m_chartlayout->setContentsMargins( 0, 0, 0, 0 );
  
  m_spectrum->setRenderHint( QPainter::Antialiasing );
  
  m_detectors->setLayout( m_detectorsLayout );
  QLabel *label = new QLabel( "Detectors: " );
  m_detectorsLayout->addWidget( label, 0, 0, Qt::AlignLeft );
  m_detectorsLayout->setColumnStretch( 1, 1 );
  m_detectorsLayout->setColumnStretch( 2, 1 );
  m_detectorsLayout->setColumnStretch( 3, 1 );
  m_detectorsLayout->setColumnStretch( 4, 1 );
  
  m_time->hide();
  m_sampleChanger->hide();
  m_detectors->hide();
  m_chartlayout->setRowStretch( 1, 0 );
  
  m_tabs->setTabPosition( QTabWidget::West );
  m_tabs->addTab( holder, "Spectrum" );
  
  m_tabs->addTab( m_detail, "Details" );
  
  m_tabs->addTab( m_save, "Save" );
  
  setCentralWidget( m_tabs );
  
  m_tabs->setCurrentIndex(0);

  QStatusBar *status = statusBar();
  
  m_loglin->setText( "Log" );
  m_loglin->setObjectName( "LogLin" );
  m_loglin->setStyleSheet( "QToolButton#LogLin"
                           "{"
                           "width: 2em;"
                           "border: none;"
                           "padding: 1px;"
                           "}"
                           "QToolButton#LogLin:hover"
                           "{"
                           "font: bold;"
                           "}"
                          );
  
  status->addWidget( m_loglin, 1 );
  status->addWidget( m_livetime, 1 );
  status->addWidget( m_realtime, 1 );
  status->addWidget( m_deadtime, 1 );
  status->addWidget( m_statusFiller, 5 );
  status->addWidget( m_mouseChannel, 1 );
  status->addWidget( m_mouseEnergy, 1 );
  status->addWidget( m_mouseChannelCounts, 1 );
  status->addWidget( m_mouseHeight, 1 );
//  m_mouseChannel->setMinimumWidth( 75 );
  status->setSizeGripEnabled( true );
  
  
  m_sampleChanger->setLayout( m_samplesLayout );
  
  m_samplesLayout->addWidget( new QWidget, 0, 0 );
  label = new QLabel( "Sample" );
  m_samplesLayout->addWidget( label, 0, 2, Qt::AlignRight );
  
  QIcon leftarrow;
  QPixmap leftpixmap;
  leftpixmap.loadFromData( left_png, left_png_len, "png" );
  leftarrow.addPixmap( leftpixmap );

  QToolButton *leftbutton = new QToolButton( m_sampleChanger );
  leftbutton->setIcon( leftarrow );
  m_samplesLayout->addWidget( leftbutton, 0, 1, Qt::AlignRight );

  // 1) Remove m_currentSampleNum and alsways use m_displayedSampleNumbers
  // 2) Make so m_sampleEdit and arrows disapear if single spectrum in file
  // 3) Copy logic from CompactFileManager::handleUserChangeSampleNum()
  //    into MainWindow::manualChangedSample()
  // 4) Make it so displayMeasurment() always simplifies the user input string
  // 5) Add tool-tip help for m_sampleEdit to explain syntax
  // 6) Add somethign similar to m_sampleEdit and detector select for the saving
  //    file widget (also, eventually the recal widget)
  
  QIntValidator *intval = new QIntValidator( -9999, 9999 );
  m_sampleEdit->setValidator( intval );
  m_samplesLayout->addWidget( m_sampleEdit, 0, 3, Qt::AlignCenter );
  m_sampleEdit->setMinimumWidth( 20 );
  m_sampleEdit->setMaxLength( 4 );
  m_sampleEdit->setMaximumWidth( 40 );
  m_sampleEdit->setAlignment( Qt::AlignCenter );
  
  QIcon rghtarrow;
  QPixmap rightpixmap;
  rightpixmap.loadFromData( right_png, right_png_len, "png" );
  rghtarrow.addPixmap( rightpixmap );

  QToolButton *rightbutton = new QToolButton( m_sampleChanger );
  rightbutton->setIcon( rghtarrow );
  m_samplesLayout->addWidget( rightbutton, 0, 5, Qt::AlignLeft );

  leftbutton->setStyleSheet( "border: none;" );
  rightbutton->setStyleSheet( "border: none;" );
  
  
  m_samplesLayout->addWidget( m_totalSamples, 0, 4, Qt::AlignLeft );
  
  
  m_samplesLayout->addWidget( new QWidget, 0, 6 );
  
  m_samplesLayout->setColumnStretch( 0, 10 );
  m_samplesLayout->setColumnStretch( 6, 10 );
  
  QMenuBar *menu = menuBar();
#if defined(__APPLE__) && !defined(IOS)
  QMenu *filemenu = menu->addMenu( "File" );
  QAction *aboutaction = filemenu->addAction( "About" );
  QAction *openaction = filemenu->addAction( "Open" );
  filemenu->addAction( aboutaction );
#else
  QAction *openaction = menu->addAction( "Open" );
  QAction *aboutaction = menu->addAction( "About" );
#endif
  
  //
  qRegisterMetaType< std::shared_ptr<MeasurementInfo> >("std::shared_ptr<MeasurementInfo>");
  
  QObject::connect( m_spectrum, SIGNAL(fileDropped(QDropEvent*)),
                   this, SLOT(recievedDropEvent(QDropEvent*)));
  
  QObject::connect( m_time, SIGNAL(fileDropped(QDropEvent*)),
                   this, SLOT(recievedDropEvent(QDropEvent*)));
  
  QObject::connect( m_spectrum, SIGNAL(mousePositionChanged(int,int,double,double,double)),
                   this, SLOT(spectrumMousePositionChanged(int,int,double,double,double)) );
  
  QObject::connect( m_spectrum, SIGNAL(mouseLeft()), this, SLOT(spectrumMouseLeft()) );
  
  
  QObject::connect( m_time, SIGNAL(mousePositionChanged(int,double,double,double,double)),
                   this, SLOT(timeMousePositionChanged(int,double,double,double,double)) );
  
  QObject::connect( m_time, SIGNAL(mouseLeft()), this, SLOT(timeMouseLeft()) );
  
  QObject::connect( m_time, SIGNAL(timeRangeSelected(int,int,Qt::KeyboardModifiers)),
                   this, SLOT(timeRangeSelected(int,int,Qt::KeyboardModifiers)) );
  
  
  QObject::connect( this, SIGNAL(displayedSpectrumChanged(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)),
                    m_save, SLOT(updateDisplay(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)) );

  QObject::connect( this, SIGNAL(displayedSpectrumChanged(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)),
                    m_detail, SLOT(updateDisplay(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)) );
  
  QObject::connect( this, SIGNAL(displayedSpectrumChanged(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)),
                    m_time, SLOT(spectrumSampleNumbersChanged(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)) );
  
  QObject::connect( m_loglin, SIGNAL(pressed()),
                    this, SLOT(toggleLogLinearYAxis()) );
  
  QObject::connect( leftbutton, SIGNAL(pressed()),
                    this, SLOT(decrementSampleNumDisplayed()) );
  QObject::connect( m_sampleEdit, SIGNAL(editingFinished()),
                   this, SLOT(manualChangedSample()) );
  QObject::connect( rightbutton, SIGNAL(pressed()),
                   this, SLOT(advanceSampleNumDisplayed()) );
  
  QObject::connect( openaction, SIGNAL(triggered(bool)),
                   this, SLOT(createOpenFileDialog()) );
  QObject::connect( aboutaction, SIGNAL(triggered (bool)),
                   this, SLOT(createAboutDialog()) );
  
  QObject::connect( m_detail, SIGNAL(fileDataModified()),
                    this, SLOT(refreshDisplays()) );
  
  QObject::connect( m_detail, SIGNAL(energyCalDialogCreated()),
                    this, SLOT(gotoChartsTab()) );

#if defined(ANDROID)
  menu->show();
#endif


#if !defined(ANDROID) && !defined(IOS)
  QSettings settings;
  const QRect screenGeom = QApplication::desktop()->screenGeometry();
  
  if( settings.contains("mainWindowGeometry")
      && settings.contains("mainWindowState") )
  {
    restoreGeometry( settings.value("mainWindowGeometry").toByteArray() );
    restoreState( settings.value("mainWindowState").toByteArray() );
  }else
  {
    const int height = static_cast<int>( 0.75*screenGeom.height() );
    const int width = static_cast<int>( 0.75*screenGeom.width() );
    
    resize( std::max(width, 400), std::max(height, 350) );
    move( height/15, width/20 );
  }//

  //Some basic sanity checks for sizing and positioning
  if( size().width() > screenGeom.width() )
    resize( screenGeom.width()-10, size().height() );
  
  if( size().height() > screenGeom.height() )
    resize( size().width(), screenGeom.height()-20 );
  
  if( pos().x() < 0 || pos().x()>(screenGeom.width()-size().width()) )
    move( 5, pos().y() );
  
  if( pos().x() < 0 || pos().y()>(screenGeom.height()-size().height()))
    move( pos().x(), 10 );
#endif  //if not Android or iOS
}//MainWindow constructor


MainWindow::~MainWindow()
{
}


void MainWindow::closeEvent( QCloseEvent *event )
{
  QSettings settings;
  settings.setValue( "mainWindowGeometry", saveGeometry() );
  settings.setValue( "mainWindowState", saveState() );
  
  QMainWindow::closeEvent(event);
}//void closeEvent( QCloseEvent *event )


void MainWindow::gotoChartsTab()
{
  m_tabs->setCurrentIndex(0);
}//void gotoChartsTab()


void MainWindow::toggleLogLinearYAxis()
{
  const bool wasLog = m_spectrum->isLogY();
  
  m_spectrum->setLogY( !wasLog );
  
  m_loglin->setText( wasLog ? "Lin" : "Log" );
}//void toggleLogLinearYAxis()


void MainWindow::createAboutDialog()
{
  QDialog dialog;
  dialog.setWindowTitle( "About Cambio" );
  QGridLayout *layout = new QGridLayout();
  dialog.setLayout( layout );
  
  QString txt = "<div align='center'><h1>Cambio</h1>";
  txt += QString("<div><b>Version %1</b></div>").arg( COMPILE_DATE_AS_INT );
  txt += "<div><b>Sandia National Laboratories</b></div>";
  txt += "</div>";
  
  QLabel *msg = 0;
  msg = new QLabel( txt );
  layout->addWidget( msg, 0, 1 );
  
  txt = "<br />"
	    "<div>"
          "Please send comments, suggestions, new file formats, and reports of"
          " problems to <a href=\"mailto:wcjohns@sandia.gov;cambio@sandia.gov?Subject=Cambio%20Lite\" target=\"_top\">wcjohns@sandia.gov AND cambio@sandia.gov</a>."
          " You are also welcome to send an email with your name and organization in order to recieve updates."
        "</div>"
        "<div>"
          "Versions of this program for other platforms, "
          "as well as the original un-maintained version of Cambio by George Lasche can obtained from "
          "<a href=\"https://hekili.ca.sandia.gov/CAMBIO/\" target=\"_top\">https://hekili.ca.sandia.gov/CAMBIO/</a>."
        "</div>";
  
  msg = new QLabel( txt );
  msg->setWordWrap( true );
  layout->addWidget( msg, 1, 0, 1, 3 );
  
  txt =  "<br />"
	     "<div align='center'><b>"
          "<div>Sandia National Laboratories</div>"
          "<div>William C. Johnson</div>"
        "</b></div>"
        "<br />"
        "<div>"
          "<div>Copyright 2014 Sandia Corporation.</div>"
        "<br />"
          "<div>This program is free software; you can redistribute it and/or</div>"
          "<div>modify it under the terms of the GNU Lesser General Public</div>"
          "<div>License as published by the Free Software Foundation; either</div>"
          "<div>version 2.1 of the License, or (at your option) any later version.</div>"
        "<br />"
          "<div>This program is distributed in the hope that it will be useful,</div>"
          "<div>but WITHOUT ANY WARRANTY; without even the implied warranty of</div>"
          "<div>MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU</div>"
          "<div>Lesser General Public License for more details.</div>"
        "<br />"
          "<div>You should have received a copy of the GNU Lesser General Public</div>"
          "<div>License along with this program; if not, write to the Free Software</div>"
          "<div>Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA</div>"
        "<br />"
        "</div>";
  
  msg = new QLabel( txt );
  layout->addWidget( msg, 2, 0, 1, 3 );

#if defined(ANDROID) || defined(IOS)
  const QRect geom = QApplication::desktop()->availableGeometry();
  dialog.resize( geom.width(), geom.height() );
#endif

  
  QPixmap cambioPixmap, snlPixmap;
  cambioPixmap.loadFromData( CambioLiteLogo_png, CambioLiteLogo_png_len );
  QLabel *imageLabel = new QLabel();
  imageLabel->setPixmap( cambioPixmap );
  layout->addWidget( imageLabel, 0, 0 );
  
  imageLabel = new QLabel();
  snlPixmap.loadFromData( SnlBirdLogo_png, SnlBirdLogo_png_len );
  imageLabel->setPixmap( snlPixmap );
  layout->addWidget( imageLabel, 0, 2 );
  
  dialog.setModal( true );
  
  QPushButton *ok = new QPushButton( "Okay" );
  layout->addWidget( ok, 3, 2, Qt::AlignRight );
  ok->setAutoDefault( true );
  
  //Should add in both the cambio and sandia logos
  
  QWidget::connect( ok, SIGNAL(clicked(bool)), &dialog, SLOT(accept()) );
  
  
  dialog.exec();
}//void createAboutDialog()


void MainWindow::createOpenFileDialog()
{
  QSettings settings;
  const QString caption = "Select file to open";
  QString initialDir = !!m_measurment && m_measurment->filename().size()
                             ? m_measurment->filename().c_str()
                             : QDir::homePath().toUtf8().data();

#if defined(ANDROID)
  if( QDir("/sdcard/Download").exists() )
      initialDir = "/sdcard/Download";
#endif

  QVariant settingsPath = settings.value( "OpenFileDir" );
  if( settingsPath.isValid() )
  {
    QString str = settingsPath.toString();
    if( str.size() > 1 && QDir(str).exists() )
      initialDir = str;
  }//if( settingsPath.isValid() )

  QFileDialog dialog( this, caption, initialDir );
  dialog.setFileMode( QFileDialog::ExistingFiles );
  dialog.setViewMode( QFileDialog::Detail );
  
#if defined(ANDROID) || defined(IOS)
  const QRect geom = QApplication::desktop()->availableGeometry();
  dialog.resize( geom.width(), geom.height() );
#endif

  const int status = dialog.exec();
  
  if( status != QDialog::Accepted )
    return;

  QStringList selectedfiles = dialog.selectedFiles();
  
  if( selectedfiles.size() == 0 )
    return;
  
  if( selectedfiles.size() == 1 )
  {
    QFileInfo info( selectedfiles[0] );
    if( info.isDir() )
    {
      m_save->initBatchConvertion( selectedfiles[0] );
      return;
    }


    QFileInfo fileinfo( selectedfiles[0] );
    settings.setValue( "OpenFileDir", fileinfo.absolutePath() );

    handleSingleFileDrop( QUrl::fromLocalFile( selectedfiles[0] ) );
    
    return;
  }//if( selectedfiles.size() == 1 )
  
  if( selectedfiles.size() > 1 )
  {
    QList<QUrl> urlist;
    for( int i = 0; i < selectedfiles.size(); ++i )
      urlist.push_back( QUrl::fromLocalFile( selectedfiles[i] ) );
    
    handleMultipleFileDrop( urlist );
  }//if( selectedfiles.size() > 1 )
}//void createOpenFileDialog();


void MainWindow::timeMouseLeft()
{
  
}//void timeMouseLeft()


void MainWindow::timeMousePositionChanged( int channel, double energy,
                                           double countheight,
                                           double channelcounts,
                                           double neutroncounts )
{
  
}//void timeMousePositionChanged(...)


void MainWindow::timeRangeSelected( int firstsample, int lastsample,
                                    Qt::KeyboardModifiers mods )
{
  if( !m_measurment )
    return;
  
  BusyIndicator *indicator = new BusyIndicator( BusyIndicator::SummingSpectra, this );
  indicator->show();
  repaint();
  indicator->repaint();
  update();
  indicator->update();
  qApp->processEvents();
  QCoreApplication::flush();
  
  if( firstsample > lastsample )
    std::swap( firstsample, lastsample );
  
  const std::set<int> &sample_numbers = m_measurment->sample_numbers();
  
  if( firstsample < 0 || lastsample >= static_cast<int>(sample_numbers.size()) )
  {
    m_displayedSampleNumbers.clear();
  }else
  {
    const vector<int> samples( sample_numbers.begin(), sample_numbers.end() );
    vector<int>::const_iterator start = samples.begin() + firstsample;
    vector<int>::const_iterator end  = samples.begin() + lastsample + 1;

    if( mods == Qt::NoModifier )
    {
      m_displayedSampleNumbers.clear();
      for( vector<int>::const_iterator i = start; i != end; ++i )
        m_displayedSampleNumbers.insert( *i );
    }else if( (mods & Qt::ControlModifier) == Qt::ControlModifier )
    {
      for( vector<int>::const_iterator i = start; i != end; ++i )
        m_displayedSampleNumbers.erase( *i );
    }else if( (mods & Qt::ShiftModifier) == Qt::ShiftModifier )
    {
      for( vector<int>::const_iterator i = start; i != end; ++i )
        m_displayedSampleNumbers.insert( *i );
    }
  }//if( invalid sample numbers ) / else
  
  if( m_displayedSampleNumbers.empty() )
    m_displayedSampleNumbers = sample_numbers;
  
  std::shared_ptr<Measurement> meas
    = m_measurment->sum_measurements( m_displayedSampleNumbers, m_detectorsDisplayed );
  
  setTimeText( meas );
  m_spectrum->setSpectrum( meas, true, m_measurment->filename().c_str() );

  emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
  
  delete indicator;
}//void timeRangeSelected(...)


void MainWindow::spectrumMouseLeft()
{
  m_mouseChannel->hide();
  m_mouseEnergy->hide();
  m_mouseChannelCounts->hide();
  m_mouseHeight->hide();
}//void spectrumMouseLeft()


void MainWindow::spectrumMousePositionChanged( int firstchannel, int lastchannel,
                                               double energy, double countheight,
                                               double channelcounts )
{
  m_mouseChannel->show();
  m_mouseEnergy->show();
  m_mouseChannelCounts->show();
  m_mouseHeight->show();
  
  char buffer[256];
  if( firstchannel == lastchannel )
    snprintf( buffer, sizeof(buffer), "channel %i,", firstchannel );
  else if( (lastchannel-firstchannel) == 1 )
    snprintf( buffer, sizeof(buffer), "channels %i,%i,",
              firstchannel, lastchannel );
  else
    snprintf( buffer, sizeof(buffer), "channel %i-%i,",
              firstchannel, lastchannel );
  m_mouseChannel->setText( buffer );
  
  
  snprintf( buffer, sizeof(buffer), " energy: %.2f keV,", energy );
  m_mouseEnergy->setText( buffer );
  
  snprintf( buffer, sizeof(buffer), " counts %.1f,", channelcounts );
  m_mouseChannelCounts->setText( buffer );
  
  snprintf( buffer, sizeof(buffer), " height %.1f", countheight );
  m_mouseHeight->setText( buffer );
}//void mousePositionChanged()



void MainWindow::recievedDropEvent( QDropEvent *event )
{
  qDebug() << "recievedDropEvent";
  
  const QMimeData *mimeData = event->mimeData();
  if( mimeData && mimeData->hasUrls() )
  {
    event->accept();
    
    QList<QUrl> urlList = mimeData->urls();
    
#ifdef Q_OS_MAC
    //Fix issue where older Qt didnt translate to the correct OSX file path
    for( int i = 0; i < urlList.size(); ++i )
      urlList[i] = fromNSUrl( urlList[i] );
#endif
    
    qDebug() << urlList << endl;
    
    if( urlList.size() == 1 )
      handleSingleFileDrop( urlList.front() );
    else if( urlList.size() > 1 )
      handleMultipleFileDrop( urlList );
  }//if( mimeData->hasUrls() )
}//void recievedDropEvent( QDropEvent * )


void MainWindow::updateForSampleNumChange()
{
  char buffer[32];
  snprintf( buffer, sizeof(buffer), "%i", m_currentSampleNum );
  m_sampleEdit->setText( buffer );
  
  const set<int> &allsamples = m_measurment->sample_numbers();
  set<int>::const_iterator pos = allsamples.find( m_currentSampleNum );
  if( pos == allsamples.end() )
  {
    qWarning() << "Logic error in MainWindow::updateForSampleNumChange()";
    return;
  }
  
  m_displayedSampleNumbers.clear();
  m_displayedSampleNumbers.insert( m_currentSampleNum );
  std::shared_ptr<Measurement> meas
             = m_measurment->sum_measurements( m_displayedSampleNumbers,
                                               m_detectorsDisplayed );
  
  string title = !!meas ? meas->title() : string("");
  if( title.empty() )
    title = m_measurment->filename();
  
  int sample = *m_displayedSampleNumbers.begin();
  snprintf( buffer, sizeof(buffer), "%i", sample );
  m_sampleEdit->setText( buffer );
  
  sample = *m_measurment->sample_numbers().rbegin();
  snprintf( buffer, sizeof(buffer), "of %i", sample );
  m_totalSamples->setText( buffer );
  
  setTimeText( meas );
  m_spectrum->setSpectrum( meas, true, title.c_str() );
  emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
}//void updateForSampleNumChange()


void MainWindow::advanceSampleNumDisplayed()
{
  const set<int> &samples = m_measurment->sample_numbers();
  set<int>::const_iterator pos = samples.find( m_currentSampleNum );
  
  if( pos == samples.end() )
  {
    m_currentSampleNum = samples.size() ? (*samples.begin()) : -1;
  }else
  {
    ++pos;
    if( pos == samples.end() )
      pos = samples.begin();
    m_currentSampleNum = *pos;
  }
 
  updateForSampleNumChange();
}//void advanceSampleNumDisplayed()


void MainWindow::decrementSampleNumDisplayed()
{
  const set<int> &samples = m_measurment->sample_numbers();
  set<int>::const_iterator pos = samples.find( m_currentSampleNum );
  
  if( pos == samples.end() )
  {
    m_currentSampleNum = samples.size() ? (*samples.begin()) : -1;
  }else
  {
    if( pos == samples.begin() )
      pos = samples.end();
    
    --pos;
    m_currentSampleNum = *pos;
  }
  
  updateForSampleNumChange();
}//void decrementSampleNumDisplayed()


void MainWindow::manualChangedSample()
{
  char buffer[32];
  string numstr = m_sampleEdit->text().toUtf8().data();
  
  int sample;
  if( !(stringstream(numstr) >> sample) )
  {
    snprintf( buffer, sizeof(buffer), "%i", m_currentSampleNum );
    m_sampleEdit->setText( buffer );
    return;
  }//if( !(stringstream(numstr) >> sample) )
  
  const set<int> &samples = m_measurment->sample_numbers();
  set<int>::const_iterator pos = samples.find( sample );
  
  if( pos == samples.end() )
  {
    snprintf( buffer, sizeof(buffer), "%i", m_currentSampleNum );
    m_sampleEdit->setText( buffer );
    return;
  }//if( pos == samples.end() )
  
  m_currentSampleNum = sample;
  updateForSampleNumChange();
}//void manualChangedSample();


void MainWindow::setTimeText( std::shared_ptr<const Measurement> meas )
{
  m_livetime->setHidden( !meas );
  m_realtime->setHidden( !meas );
  m_deadtime->setHidden( !meas );
  
  if( !meas )
    return;
  
  const double rt = meas->real_time();
  const double lt = meas->live_time();
  
  double deadtime = -1.0;
  if( rt > 0.0 && lt > 0.0 && !IsInf(rt) && !IsNan(rt) && !IsInf(lt) && !IsNan(lt)  )
    deadtime = 100.0*(rt-lt) / rt;
  
  char buffer[128];
  snprintf( buffer, sizeof(buffer), "Live Time: %.1fs,", lt );
  m_livetime->setText( buffer );
  
  snprintf( buffer, sizeof(buffer), "Real Time: %.1fs, ", rt );
  m_realtime->setText( buffer );
  
  if( deadtime >= 0.0 )
  {
    m_deadtime->show();
    snprintf( buffer, sizeof(buffer), "Dead Time: %.1f%%", deadtime );
    m_deadtime->setText( buffer );
  }else
    m_deadtime->hide();
}//void setTimeText( std::shared_ptr<Measurement> meas )


void MainWindow::calculateTimeSeriesData(
                            std::shared_ptr<Measurement> &grosscounts ) const
{
  //for 'rslbig.n42' this function takes about 165 ms, so perhaps mutlithreading
  //  might be helpful?
  
  if( !m_measurment || !m_measurment->passthrough() )
    return;
  
  const vector<int> det_nums = m_measurment->detector_numbers();
  const vector<bool> &det_to_use = m_detectorsDisplayed;

  if( det_nums.size() != det_to_use.size() )
    throw runtime_error( "Inconsistent number of detectors." );

  set<int> dets;
  for( size_t i = 0; i < det_nums.size(); ++i )
    if( det_to_use[i] )
      dets.insert( det_nums[i] );
  
  const std::set<int> &sample_numbers = m_measurment->sample_numbers();
  
  double time = 0.0;
  vector<int> sample_num_vec;
  vector<pair<float,int> > binning;
  binning.push_back( pair<float,int>(0.0f,-1) );
  
  foreach( const int sample, sample_numbers )
  {
    const vector< MeasurementConstShrdPtr > meas
                                  = m_measurment->sample_measurements( sample );
    
    if( meas.empty() )
      continue;
    
    foreach( const MeasurementConstShrdPtr &m, meas )
    {
      if( !dets.count( m->detector_number() ) )
        continue;
      
      if( m->source_type() == Measurement::Background )
        continue;
      
      time += (m->real_time()/meas.size());
    }
    
    sample_num_vec.push_back( sample );
    binning.push_back( make_pair(static_cast<float>(time),sample) );
  }//foreach( const int sample, sample_nums )
  
  sort( binning.begin(), binning.end() );
  
  vector<float> bin_edges( binning.size() );
  for( size_t i = 0; i < binning.size(); ++i )
    bin_edges[i] = binning[i].first;
  const int num_time_bins = static_cast<int>( bin_edges.size() );
  if( num_time_bins > 2 )
    bin_edges.push_back( 2.0f*bin_edges.back() - bin_edges[num_time_bins-2]);
  else if( num_time_bins )
    bin_edges.push_back( bin_edges.back() + 1.0f );
  
  grosscounts.reset( new Measurement() );
//  std::shared_ptr<Measurement> grosscounts( new Measurement() );
  
  std::shared_ptr< vector<float> > times( new vector<float>(bin_edges) );
  
  const size_t ntimes = times->size();
  
  std::shared_ptr< vector<float> > gamma_counts( new vector<float>(ntimes) );
  std::shared_ptr< vector<float> > nuetron_counts( new vector<float>(ntimes) );
  
  foreach( const int sample_number, sample_numbers )
  {
    vector<int>::const_iterator begin = sample_num_vec.begin();
    vector<int>::const_iterator end = sample_num_vec.end();
    vector<int>::const_iterator pos = std::find( begin, end, sample_number );
    
    if( pos == end )
      continue;
    
    int bin = static_cast<int>(pos - begin) + 1;  //XXX - I'm not totally sure of this
    
    const vector< MeasurementConstShrdPtr > meas
                         = m_measurment->sample_measurements( sample_number );
    
    float num_gamma = 0.0f, num_nuteron = 0.0f, live_time = 0.0f;
    for( size_t i = 0; i < meas.size(); ++i )
    {
      if( !meas[i] )
        continue;  //shouldnt ever happen
      
      if( !dets.count( meas[i]->detector_number() ) )
        continue;
      
      live_time += meas[i]->live_time();
      num_gamma += meas[i]->gamma_count_sum();
      num_nuteron += meas[i]->neutron_counts_sum();
    }//for( size_t i = 0; i < meas.size(); ++i )
    
    if( live_time > 0.0f )
      num_gamma /= (live_time / meas.size());
    gamma_counts->operator[](bin-1) += num_gamma;
    nuetron_counts->operator[](bin-1) += num_nuteron;
  }//foreach( const int sample_number, sample_numbers )
  
  
  grosscounts->set_channel_energies( times );
  grosscounts->set_gamma_counts( gamma_counts, 0.0f, 0.0f );
  grosscounts->set_neutron_counts( *nuetron_counts );
}//void MainWindow::calculateTimeSeriesData()


void MainWindow::displayMeasurment()
{
  if( !m_measurment )
  {
    std::shared_ptr<Measurement> dummy;
    
    setTimeText( dummy );
    m_spectrum->setSpectrum( dummy, true, "" );
    m_time->setGrossCounts( dummy );
    
    m_time->hide();
    m_chartlayout->setRowStretch( 1, 0 );
    
    m_sampleChanger->hide();
    m_detectors->hide();
    
    emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
  }else if( m_measurment->passthrough() )
  {
    BusyIndicator *indicator = new BusyIndicator( BusyIndicator::SummingSpectra, this );
    indicator->show();
    repaint();
    indicator->repaint();
    update();
    indicator->update();
    qApp->processEvents();
    QCoreApplication::flush();
    
    m_time->show();
    m_chartlayout->setRowStretch( 1, 2 );
    
    m_sampleChanger->hide();
    
    std::shared_ptr<Measurement> meas
                   = m_measurment->sum_measurements( m_displayedSampleNumbers,
                                                     m_detectorsDisplayed );
    
    qDebug() << "NumNeutrons=" << meas->neutron_counts_sum() << endl;
    
    if( !meas && m_measurment->gamma_channel_counts().size() > 1 )
    {
      QDialog dialog;
      dialog.setWindowTitle( "Multiple Binnings" );
      QGridLayout *layout = new QGridLayout();
      dialog.setLayout( layout );
      QLabel *label = new QLabel( "Spectra with varying number of channels found."
                                  " Please select the number of channels you'de like "
                                  "to display spectra of" );
      label->setWordWrap( true );
      layout->addWidget( label, 0, 0 );
      
      const set<size_t> nchannel = m_measurment->gamma_channel_counts();
      
      QSignalMapper *mapper = new QSignalMapper( &dialog );
      
      foreach( size_t n, nchannel )
      {
        char buffer[64];
        snprintf( buffer, sizeof(buffer), "%i Channels", int(n) );
        QPushButton *b = new QPushButton( buffer );
        layout->addWidget( b, layout->rowCount(), 0, Qt::AlignJustify );
        QWidget::connect( b, SIGNAL(pressed()), mapper, SLOT(map()) );
        mapper->setMapping( b, int(n) );
      }
      
      QWidget::connect( mapper, SIGNAL(mapped(int)), &dialog, SLOT(done(int))) ;
      
      const int nbin = dialog.exec();

      m_measurment->keep_n_bin_spectra_only( static_cast<size_t>(nbin) );
      meas = m_measurment->sum_measurements( m_displayedSampleNumbers,
                                             m_detectorsDisplayed );
      
      //Still not quite right - needs a cleanup probably
    }//if( !meas && m_measurment->gamma_channel_counts().size() > 1 )
    
    
    setTimeText( meas );
    m_spectrum->setSpectrum( meas, true, m_measurment->filename().c_str() );
    
    //Suprisingly, it doesnt save any time to call calculateTimeSeriesData(...)
    //  in another thread while setTimeText() and setSpectrum() are being
    //  called.
    std::shared_ptr<Measurement> grosscounts;
    calculateTimeSeriesData( grosscounts );
    
    m_time->setGrossCounts( grosscounts );
    
    emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
    delete indicator;
  }else if( m_measurment->sample_numbers().size() == 1 )
  {
    m_time->hide();
    m_chartlayout->setRowStretch( 1, 0 );
    
    m_sampleChanger->hide();
    
    std::shared_ptr<Measurement> meas
              = m_measurment->sum_measurements( m_displayedSampleNumbers, m_detectorsDisplayed );
    
    string title = !!meas ? meas->title() : string("");
    if( title.empty() )
    {
      QFileInfo info( m_measurment->filename().c_str() );
      title = info.fileName().toUtf8().data();
    }

    if( m_measurment->measurements().size() == 1
       && m_measurment->measurements()[0]->title().size()>1 )
      title = m_measurment->measurements()[0]->title();
    
    setTimeText( meas );
    m_spectrum->setSpectrum( meas, true, title.c_str() );
    emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
  }else
  {
    m_time->hide();
    m_chartlayout->setRowStretch( 1, 0 );
    
    m_sampleChanger->show();
    
    m_displayedSampleNumbers.clear();
    m_displayedSampleNumbers.insert( *(m_measurment->sample_numbers().begin()) );
    std::shared_ptr<Measurement> meas
             = m_measurment->sum_measurements( m_displayedSampleNumbers, m_detectorsDisplayed );
    
    string title = !!meas ? meas->title() : string("");
    if( title.empty() )
      title = m_measurment->filename();
    
    char buffer[32];
    int sample = *m_displayedSampleNumbers.begin();
    snprintf( buffer, sizeof(buffer), "%i", sample );
    m_sampleEdit->setText( buffer );
    
    m_currentSampleNum = sample;
    
    sample = *m_measurment->sample_numbers().rbegin();
    snprintf( buffer, sizeof(buffer), "of %i", sample );
    m_totalSamples->setText( buffer );
    
    setTimeText( meas );
    m_spectrum->setSpectrum( meas, true, title.c_str() );
    emit displayedSpectrumChanged(m_measurment,m_displayedSampleNumbers,m_detectorsDisplayed);
  }//if( !m_measurment ) / else / else
  
  for( size_t i = 0; i < m_detCheckBox.size(); ++i )
    delete m_detCheckBox[i];
  m_detCheckBox.clear();
  
  if( m_detectorsDisplayed.size() < 2 )
  {
    m_detectors->hide();
  }else
  {
    for( size_t i = 0; i < m_detectorsDisplayed.size(); ++i )
    {
      QCheckBox *cb = new QCheckBox( m_measurment->detector_names()[i].c_str() );
      cb->setChecked( m_detectorsDisplayed[i] );
      
      const int row = i / 4;
      const int col = 1 + (i % 4);
      m_detectorsLayout->addWidget( cb, row, col );
      
      m_detCheckBox.push_back( cb );
      QWidget::connect( cb, SIGNAL(stateChanged(int)), this, SLOT(detectorsToDisplayChanged()) );
    }//for( size_t i = 0; i < m_detectorsDisplayed.size(); ++i )
    
    m_detectors->show();
  }//if( m_detectorsDisplayed.size() < 2 ) / else
}//void loadMeasurment();


void MainWindow::refreshDisplays()
{
  displayMeasurment();
}//void refreshDisplays()


void MainWindow::detectorsToDisplayChanged()
{
  assert( m_detectorsDisplayed.size() == m_detCheckBox.size() );
  
  for( size_t i = 0; i < m_detectorsDisplayed.size(); ++i )
    m_detectorsDisplayed[i] = m_detCheckBox[i]->isChecked();
  
  displayMeasurment();
}//void detectorsToDisplayChanged()


void MainWindow::setMeasurment( std::shared_ptr<MeasurementInfo> measurment )
{
  m_measurment = measurment;

  if( !!m_measurment
      && (m_measurment->measurements().empty()
          || m_measurment->sample_numbers().empty()) )
    m_measurment.reset();
  
  m_livetime->setHidden( !m_measurment );
  m_realtime->setHidden( !m_measurment );
  m_deadtime->setHidden( !m_measurment );
  
  m_detectorsDisplayed.clear();
  if( !!m_measurment )
    m_detectorsDisplayed.resize( m_measurment->detector_names().size(), true );
  
  m_displayedSampleNumbers.clear();
  if( !!m_measurment )
    m_displayedSampleNumbers = m_measurment->sample_numbers();
  
//  m_save->setMeasurment( m_measurment );
//  m_detail->setMeasurment( m_measurment );

  if( !!m_measurment && m_measurment->passthrough() )
  {
    QString msg = "Click and drag time series to change spectrum."
                  " Hold Shift to add or Right mouse button to remove.";
    statusBar()->showMessage( msg, 2000 );
  }else
  {
    
  }//if( is passthrough/searchmode ) / else
  
  displayMeasurment();
}//void setMeasurment( std::shared_ptr<MeasurementInfo> measurment )



void MainWindow::handleSingleFileDrop( const QUrl &url )
{
  const QString filename = url.toLocalFile();
  QFileInfo fileinfo( filename );
  
  if( fileinfo.isDir() )
  {
    m_save->initBatchConvertion( filename );
    return;
  }//if( fileinfo.isDir() )
 
  
  BusyIndicator *indicator = 0;
  if( fileinfo.size() > 30*1024 )
  {
    indicator = new BusyIndicator( BusyIndicator::OpeningFile, this );
    indicator->show();
    repaint();
    indicator->repaint();
    update();
    indicator->update();
    qApp->processEvents();
    QCoreApplication::flush();
  }
  
  std::shared_ptr<MeasurementInfo> info = std::make_shared<MeasurementInfo>();
  const bool open = info->load_file( filename.toUtf8().data(), kAutoParser );
  
  if( indicator )
    delete indicator;
  indicator = 0;
  qApp->processEvents();
  QCoreApplication::flush();
    
  if( open )
  {
    info->set_filename( UtilityFunctions::filename( info->filename() ) );
    
    setMeasurment( info );
  }else
  {
    QString msg = "Unable to open file.  If you think this is a valid"
                  " spectrum file, please email it to wcjohns@sandia.gov"
                  " to fix this.";
    
    QMessageBox msgBox;
    msgBox.setText( msg );
    msgBox.exec();
  }
}//void handleSingleFileDrop( QUrl url )


void MainWindow::handleMultipleFileDrop( const QList<QUrl> &urlist )
{
  QStringList pathList;
  for( int i = 0; i < urlist.size(); ++i )
    pathList.append(urlist.at(i).toLocalFile());
  m_save->initBatchConvertion( pathList );
}//void handleMultipleFileDrop( QList<QUrl> urlist );


bool MainWindow::eventFilter( QObject *o, QEvent *e )
{
  if( e->type()==QEvent::DragEnter )
  {
    e->accept();
//    QDragEnterEvent *event = dynamic_cast<QDragEnterEvent *>( e );
//    if( event )
//      event->acceptProposedAction();
    return true;
  }else if( e->type()==QEvent::DragLeave )
  {
    e->accept();
//    QDragLeaveEvent *event = dynamic_cast<QDragLeaveEvent *>( e );
//    if( event )
//      event->accept();
    return true;
  }else if( e->type()==QEvent::Drop )
  {
    QDropEvent *event = dynamic_cast<QDropEvent *>( e );
    
    if( event )
    {
      qDebug() << "QEvent::Drop" << event << endl;
      
      const QMimeData *mimeData = event->mimeData();
      if( mimeData && mimeData->hasUrls() )
      {
        QStringList pathList;
        QList<QUrl> urlList = mimeData->urls();
        
#ifdef Q_OS_MAC
        //Fix issue where older Qt didnt translate to the correct OSX file path
        for( int i = 0; i < urlList.size(); ++i )
          urlList[i] = fromNSUrl( urlList[i] );
#endif

        // extract the local paths of the files
        for( int i = 0; i < urlList.size(); ++i )
          pathList.append(urlList.at(i).toLocalFile());
        
        // call a function to open the files
//         openFiles(pathList);
        
        event->acceptProposedAction();
      }//if( mimeData->hasUrls() )
      
      return true;
    }//if( event )
  }
  
  return false;
}//bool eventFilter( QObject *o, QEvent *e )


void MainWindow::dragEnterEvent( QDragEnterEvent *event )
{
  //qDebug() << "dragEnterEvent: ";
  event->acceptProposedAction();

  /*
  const QMimeData *mimeData = event->mimeData();
  if( mimeData->hasText() )
    qDebug() << "\tText: " << mimeData->text();
  if( mimeData->hasHtml() )
    qDebug() << "\tHTML: " << mimeData->html();
  if( mimeData->hasUrls() )
    qDebug() << "\tURLs";
  if( mimeData->hasImage() )
    qDebug() << "\tImage";
  for( int i = 0; i < mimeData->formats().size(); i++ )
    qDebug() << "\tFormat " << i << ": " << mimeData->formats()[i];
  if( mimeData->hasColor() )
    qDebug() << "\tColor";
  */
}//void dragEnterEvent( QDragEnterEvent *event )


void MainWindow::dropEvent( QDropEvent *event )
{
  qDebug() << "MainWindow::dropEvent: ";// << event->mimeData()->formats().join(",");
  
  const QMimeData *mimeData = event->mimeData();

  if( mimeData && mimeData->hasFormat("FileContents") )
  {
    //Dragging a file from Outlook on windows seems to be the only thing that
    //  makes it here
    string tmpfilename;
    
    try
    {
      //On Windows when draggin from outlook, we get the following datas:
      //  "FileGroupDescriptorW", "FileGroupDescriptor", "RenPrivateItem",
      //  "FileContents"
      //  However, only FileContents seems to have anything in it.
      QByteArray contents = mimeData->data("FileContents");
      if( !contents.size() )
        throw runtime_error( "No File Contents" );

      const string temporarydir = UtilityFunctions::temp_dir();
      tmpfilename = UtilityFunctions::temp_file_name( "cambio", temporarydir );
    
      {
        ofstream file( tmpfilename.c_str(), ios::binary | ios::out );
        if( !file.write( contents.data(), contents.size() ) )
          throw std::runtime_error( "Failed to write: " + tmpfilename );
      }
	  
      handleSingleFileDrop( QUrl::fromLocalFile( tmpfilename.c_str() ) );
      event->acceptProposedAction();
    }catch( std::exception &e )
    {
      qDebug() << "dropEvent() caught: " << e.what();
    }
	
    try
    {
      if( !tmpfilename.empty() )
        UtilityFunctions::remove_file( tmpfilename );
    }catch( std::exception &e )
    {
      qDebug() << "dropEvent() failed to delete: " << tmpfilename.c_str() 
		       << ", exception: " << e.what();
    }
  }else if( mimeData && mimeData->hasUrls() )
  {
    event->accept();
    
    QList<QUrl> urlList = mimeData->urls();
    
#ifdef Q_OS_MAC
    //Fix issue where older Qt didnt translate to the correct OSX file path
    for( int i = 0; i < urlList.size(); ++i )
      urlList[i] = fromNSUrl( urlList[i] );
#endif

    
    if( urlList.size() == 1 )
      handleSingleFileDrop( urlList.front() );
    else if( urlList.size() > 1 )
      handleMultipleFileDrop( urlList );
  }
  
  //if( mimeData->hasFormat("FileContents") )
 
  
}//void dropEvent( QDropEvent *event )


void MainWindow::dragMoveEvent( QDragMoveEvent *event )
{
//  qDebug() << "dragMoveEvent" << endl;
  event->acceptProposedAction();
}//void dragMoveEvent(QDragMoveEvent *)


void MainWindow::dragLeaveEvent( QDragLeaveEvent *event )
{
  event->accept();
  qDebug() << "dragLeaveEvent" << endl;
}//void dragLeaveEvent(QDragLeaveEvent *)


