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


#include <QLabel>
#include <QDebug>
#include <QDialog>
#include <QWidget>
#include <QSpinBox>
#include <QTimeZone>
#include <QLineEdit>
#include <QGroupBox>
#include <QTextEdit>
#include <QPushButton>
#include <QGridLayout>
#include <QRadioButton>
#include <QButtonGroup>
#include <QDateTimeEdit>
#include <QIntValidator>
#include <QSignalMapper>
#include <QDoubleValidator>

#define BOOST_DATE_TIME_NO_LIB
#include <boost/date_time/posix_time/posix_time.hpp>

#include <boost/lexical_cast.hpp>

#include "cambio/FileDetailTools.h"
#include "cambio/FileDetailWidget.h"
#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"

using namespace std;

namespace
{
  QDateTime posix_to_qt( const boost::posix_time::ptime &time )
  {
    static const boost::posix_time::ptime epoch(boost::gregorian::date(1970,1,1));
    const boost::posix_time::time_duration::sec_type x = (time - epoch).total_seconds();
    const boost::posix_time::time_duration::fractional_seconds_type fracs = time.time_of_day().fractional_seconds();
    const int nmilli = (1000 * fracs) / boost::posix_time::time_duration::ticks_per_second();
  
    QDateTime dt;
	dt.setTimeSpec( Qt::UTC );
    dt.setTime_t( x );
    dt.addMSecs( nmilli );
  
    return dt;
  }//QDateTime posix_to_qt( const boost::posix_time::ptime &time )


  boost::posix_time::ptime qt_to_posix( const QDateTime &dt )
  {
    boost::gregorian::date date( dt.date().year(), dt.date().month(), dt.date().day() );
    const int msec = dt.time().msec() * boost::posix_time::time_duration::ticks_per_second() / 1000;
    boost::posix_time::time_duration td( dt.time().hour(), dt.time().minute(), dt.time().second(), msec );
  
    return boost::posix_time::ptime( date, td );
  }//boost::posix_time::ptime qt_to_posix( dt )
}//namespace


FileDetailWidget::FileDetailWidget( QWidget *parent, Qt::WindowFlags f )
  : QWidget( parent, f )
{
  QGridLayout *layout = new QGridLayout;
  setLayout( layout );
  
  QGroupBox *filebox = new QGroupBox();
  QGroupBox *specbox = new QGroupBox();
  QGroupBox *alterbox = new QGroupBox();
  
  filebox->setTitle( "File Specific Parameters" );
  specbox->setTitle( "Record Specific Parameters" );
  alterbox->setTitle( "Data Edit Tools" );
  
  layout->addWidget( filebox, 0, 0 );
  layout->addWidget( specbox, 1, 0 );
  layout->addWidget( alterbox, 2, 0 );
  
  layout->setRowStretch( 0, 1 );
  layout->setRowStretch( 1, 1 );
  
  QGridLayout *filelayout = new QGridLayout();
  filebox->setLayout( filelayout );
  
  QGridLayout *speclayout = new QGridLayout();
  specbox->setLayout( speclayout );
  
  QGridLayout *alterlayout = new QGridLayout();
  alterbox->setLayout( alterlayout );
  
  
  QLabel *label = new QLabel( "File Name" );
  filelayout->addWidget( label, 0, 0 );
  
  m_filename = new QLineEdit;
  filelayout->addWidget( m_filename, 0, 1, 1, 5 );
  
  label = new QLabel( "UUID" );
  filelayout->addWidget( label, 1, 0 );
  m_uuid = new QLineEdit;
  filelayout->addWidget( m_uuid, 1, 1 );
  
  label = new QLabel( "Inspection" );
  filelayout->addWidget( label, 1, 2 );
  m_inspection = new QLineEdit;
  filelayout->addWidget( m_inspection, 1, 3 );
  
  label = new QLabel( "Lane" );
  filelayout->addWidget( label, 1, 4 );
  m_lane = new QLineEdit;
  QIntValidator *intval = new QIntValidator( 0, 1000000 );
  m_lane->setValidator( intval );
  filelayout->addWidget( m_lane, 1, 5 );
  
  label = new QLabel( "Location" );
  filelayout->addWidget( label, 2, 0 );
  m_location = new QLineEdit;
  filelayout->addWidget( m_location, 2, 1 );
  
  label = new QLabel( "Inst. Type" );
  filelayout->addWidget( label, 2, 2 );
  m_instType = new QLineEdit;
  filelayout->addWidget( m_instType, 2, 3 );
  
  label = new QLabel( "Manufacturer" );
  filelayout->addWidget( label, 2, 4 );
  m_manufacturer = new QLineEdit;
  filelayout->addWidget( m_manufacturer, 2, 5 );
  
  label = new QLabel( "Model" );
  filelayout->addWidget( label, 3, 0 );
  m_model = new QLineEdit;
  filelayout->addWidget( m_model, 3, 1 );
  
  label = new QLabel( "Serial #" );
  filelayout->addWidget( label, 3, 2 );
  m_serial = new QLineEdit;
  filelayout->addWidget( m_serial, 3, 3 );
  
  
  label = new QLabel( "Mem Size (kb)" );
  filelayout->addWidget( label, 3, 4 );
  m_memsize = new QLabel;
  filelayout->addWidget( m_memsize, 3, 5 );
  
  label = new QLabel( "File" );
  filelayout->addWidget( label, 4, 0 );
  
  label = new QLabel( "Remarks" );
  filelayout->addWidget( label, 5, 0 );
  
  m_fileComments = new QTextEdit;
  filelayout->addWidget( m_fileComments, 4, 1, 4, 5 );
  m_fileComments->setAcceptRichText( false );
  
  label = new QLabel( "Date/Time" );
  speclayout->addWidget( label, 0, 0 );
  m_date = new QDateTimeEdit;
  speclayout->addWidget( m_date, 0, 1, 1, 3 );
  
  label = new QLabel( "Real Time (s)" );
  speclayout->addWidget( label, 0, 4 );
  m_realtime = new QLineEdit;
  QDoubleValidator *validator = new QDoubleValidator( 0, DBL_MAX, 2 );
  m_realtime->setValidator( validator );
  speclayout->addWidget( m_realtime, 0, 5 );
  
  label = new QLabel( "Live Time (s)" );
  speclayout->addWidget( label, 0, 6 );
  m_livetime = new QLineEdit;
  validator = new QDoubleValidator( 0, DBL_MAX, 2 );
  m_livetime->setValidator( validator );
  speclayout->addWidget( m_livetime, 0, 7 );
  
  label = new QLabel( "Latitude" );
  speclayout->addWidget( label, 1, 0 );
  m_latitude = new QLineEdit;
  validator = new QDoubleValidator( -90.0, 90.0, 7 );
  m_latitude->setValidator( validator );
  speclayout->addWidget( m_latitude, 1, 1 );
  
  label = new QLabel( "Latitude" );
  speclayout->addWidget( label, 1, 2 );
  m_longitude = new QLineEdit;
  validator = new QDoubleValidator( -180.0, 180.0, 7 );
  m_longitude->setValidator( validator );
  speclayout->addWidget( m_longitude, 1, 3 );
  
  label = new QLabel( "Pos. Date/Time" );
  speclayout->addWidget( label, 1, 4 );
  m_posdate = new QDateTimeEdit;
  speclayout->addWidget( m_posdate, 1, 5, 1, 3 );
  
  label = new QLabel( "Neutron Count" );
  speclayout->addWidget( label, 2, 0 );
  m_neutroncount = new QLabel;
  speclayout->addWidget( m_neutroncount, 2, 1 );
  
  label = new QLabel( "Gamma Count" );
  speclayout->addWidget( label, 2, 2 );
  m_gammacount = new QLabel;
  speclayout->addWidget( m_gammacount, 2, 3 );
  
  label = new QLabel( "Num. Channels" );
  speclayout->addWidget( label, 2, 4 );
  m_numchannels = new QLabel;
  speclayout->addWidget( m_numchannels, 2, 5 );
  
  label = new QLabel( "Energy (keV)" );
  speclayout->addWidget( label, 2, 6 );
  m_energy = new QLabel;
  speclayout->addWidget( m_energy, 2, 7 );
  
  label = new QLabel( "Title" );
  speclayout->addWidget( label, 3, 0 );

  label = new QLabel( "Record" );
  speclayout->addWidget( label, 4, 0 );
  
  label = new QLabel( "Comments" );
  speclayout->addWidget( label, 5, 0 );
  
  m_specTitle = new QLineEdit;
  speclayout->addWidget( m_specTitle, 3, 1, 1, 7 );

  m_specComments = new QTextEdit;
  m_specComments->setAcceptRichText( false );
  speclayout->addWidget( m_specComments, 4, 1, 2, 7 );
  
  label = new QLabel( "Record" );
  speclayout->addWidget( label, 8, 2, Qt::AlignRight );
  m_record = new QSpinBox;
  speclayout->addWidget( m_record, 8, 3 );
  m_numrecords = new QLabel;
  speclayout->addWidget( m_numrecords, 8, 4 );
  
  
  QPushButton *combineChannels = new QPushButton( "Combine Channels" );
  combineChannels->setToolTip( "Allows you to combine channels together to"
                               " reduce the total number of channels in the"
                               " spectrum or file" );
  alterlayout->addWidget( combineChannels, 0, 0, Qt::AlignLeft );
  
  QPushButton *cropChannels = new QPushButton( "Crop Energy" );
  cropChannels->setToolTip( "Allows you to change either the lower or upper"
                            " energy or channel the spectrum or file will"
                            " keep" );
  alterlayout->addWidget( cropChannels, 0, 1, Qt::AlignLeft );
  

  QPushButton *energyCal = new QPushButton( "Energy Cal (not complete)" );
  energyCal->setToolTip( "Under Construction - allows you to view/change energy callibration" );
  alterlayout->addWidget( energyCal, 0, 1, Qt::AlignLeft );
  

  QWidget *spacer = new QWidget();
  alterlayout->addWidget( spacer, 0, 3 );
  alterlayout->setColumnStretch( 2, 10 );
  
  QObject::connect( m_fileComments, SIGNAL(textChanged()), this, SLOT(fileCommentsChanged()) );
  QObject::connect( m_specTitle, SIGNAL(editingFinished()), this, SLOT(specTitleChanged()) );
  QObject::connect( m_specComments, SIGNAL(textChanged()), this, SLOT(specCommentsChanged()) );
  QObject::connect( m_filename, SIGNAL(editingFinished()), this, SLOT(filenameChanged()) );
  QObject::connect( m_uuid, SIGNAL(editingFinished()), this, SLOT(uuidChanged()) );
  QObject::connect( m_inspection, SIGNAL(editingFinished()), this, SLOT(inspectionChanged()) );
  QObject::connect( m_lane, SIGNAL(editingFinished()), this, SLOT(laneChanged()) );
  QObject::connect( m_location, SIGNAL(editingFinished()), this, SLOT(locationChanged()) );
  QObject::connect( m_instType, SIGNAL(editingFinished()), this, SLOT(instTypeChanged()) );
  QObject::connect( m_manufacturer, SIGNAL(editingFinished()), this, SLOT(manufacturerChanged()) );
  QObject::connect( m_model, SIGNAL(editingFinished()), this, SLOT(modelChanged()) );
  QObject::connect( m_serial, SIGNAL(editingFinished()), this, SLOT(serialChanged()) );
  QObject::connect( m_realtime, SIGNAL(editingFinished()), this, SLOT(realtimeChanged()) );
  QObject::connect( m_livetime, SIGNAL(editingFinished()), this, SLOT(livetimeChanged()) );
  QObject::connect( m_latitude, SIGNAL(editingFinished()), this, SLOT(latitudeChanged()) );
  QObject::connect( m_longitude, SIGNAL(editingFinished()), this, SLOT(longitudeChanged()) );
  
  QObject::connect( m_date, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(dateTimeChanged()) );
  QObject::connect( m_posdate, SIGNAL(dateTimeChanged(const QDateTime&)), this, SLOT(posDateTimeChanged()) );
  
  QObject::connect( m_record, SIGNAL(valueChanged(int)), this, SLOT(changeRecord(int)) );
  
  QObject::connect( combineChannels, SIGNAL(clicked(bool)), this, SLOT(createCombineChannelDialog()) );
  QObject::connect( cropChannels, SIGNAL(clicked(bool)), this, SLOT(createCropChannelDialog()) );
  QObject::connect( energyCal, SIGNAL(clicked(bool)), this, SLOT(createEnergyCalDialog()) );
  
}//FileDetailWidget constructor


FileDetailWidget::~FileDetailWidget()
{
  
}//FileDetailWidget destructor


void FileDetailWidget::changeRecord( int record )
{
  const bool valid = (record>0 && !!m_measurment
                      && record<=static_cast<int>(m_measurment->measurements().size()));

  m_date->setEnabled( valid );
  m_posdate->setEnabled( valid );
  m_realtime->setEnabled( valid );
  m_livetime->setEnabled( valid );
  m_latitude->setEnabled( valid );
  m_longitude->setEnabled( valid );
  m_specTitle->setEnabled( valid );
  m_specComments->setEnabled( valid );
  m_neutroncount->setEnabled( valid );
  m_gammacount->setEnabled( valid );
  m_numchannels->setEnabled( valid );
  m_energy->setEnabled( valid );
  
  if( !valid )
  {
    m_realtime->setText("");
    m_livetime->setText("");
    m_latitude->setText("");
    m_longitude->setText("");
	m_specTitle->setText("");
    m_specComments->setText("");
    m_neutroncount->setText("");
    m_gammacount->setText("");
    m_numchannels->setText("");
    m_energy->setText("");

    return;
  }//if( !valid )
  
  char buffer[128];
  MeasurementConstShrdPtr meas = m_measurment->measurements()[record-1];
  m_meas = meas;
  
  QDateTime startime;
  if( !meas->start_time().is_special() )
    startime = posix_to_qt( meas->start_time() );
  
  m_date->setDateTime( startime );
  
  snprintf( buffer, sizeof(buffer), "%.3f", meas->live_time() );
  m_livetime->setText( buffer );
  
  snprintf( buffer, sizeof(buffer), "%.3f", meas->real_time() );
  m_realtime->setText( buffer );
  
  if( meas->has_gps_info() )
  {
    snprintf( buffer, sizeof(buffer), "%.6f", meas->longitude() );
    m_longitude->setText( buffer );
    snprintf( buffer, sizeof(buffer), "%.6f", meas->latitude() );
    m_latitude->setText( buffer );
    
    QDateTime dt;
    if( !meas->position_time().is_special() )
      dt = posix_to_qt( meas->position_time() );
    m_posdate->setDateTime( dt );
  }else
  {
    QDateTime dt;
    m_posdate->setDateTime( dt );
    m_latitude->setText( "" );
    m_longitude->setText( "" );
  }//if( meas->hasGpsInfo() ) / else
  
  m_specTitle->setText( meas->title().c_str() );

  QString remarkstr;
  const vector<string> &remark = meas->remarks();
  
  for( size_t i = 0; i < remark.size(); ++i )
    remarkstr += (i?"\n":"") + QString(remark[i].c_str());
  //m_specComments->append( remark[i].c_str() );

  //This next line apparently causes the textCHnaged() signal to be
  //  emitted, which is less than ideal since it actually parses the
  //  remarks from the GUI, and places them back in the Measurment...
  m_specComments->setText( remarkstr );

  if( meas->contained_neutron() )
  {
    snprintf( buffer, sizeof(buffer), "%.g", meas->neutron_counts_sum() );
    m_neutroncount->setText( buffer );
  }else
  {
    m_neutroncount->setText( "" );
  }
  
  snprintf( buffer, sizeof(buffer), "%.6g", meas->gamma_count_sum() );
  m_gammacount->setText( buffer );
  
  size_t nbin = 0;
  if( !!meas->gamma_counts() )
  {
    nbin = int(meas->gamma_counts()->size());
    snprintf( buffer, sizeof(buffer), "%i", int(nbin) );
    m_numchannels->setText( buffer );
  }else
  {
    m_numchannels->setText( "" );
  }
  
  if( !!meas->channel_energies() && nbin>3 )
  {
    const double lowe = meas->channel_energies()->at(0);
    const double laste = meas->channel_energies()->at(nbin-1);
    const double penultimate = meas->channel_energies()->at(nbin-2);
    const double uppere = 2*laste - penultimate;
    snprintf( buffer, sizeof(buffer), "%.2f-%.2f", lowe, uppere );
    m_energy->setText( buffer );
  }else
  {
    m_energy->setText( "" );
  }
  
  m_record->setValue( record );
}//void changeRecord( int record );


void FileDetailWidget::specTitleChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  const QString txt = m_specTitle->text();
  m_measurment->set_title( txt.toUtf8().data(), m_meas );
}//void specTitleChanged()


void FileDetailWidget::specCommentsChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  qDebug() << "In specCommentsChanged";

  vector<string> remarks;
  const QString txt = m_specComments->toPlainText();
  const QStringList	strings = txt.split ( "\n", QString::SkipEmptyParts );
  for( int i = 0 ; i < strings.length(); ++i )
    remarks.push_back( strings[i].toUtf8().data() );
  m_measurment->set_remarks( remarks, m_meas );
}//void specCommentsChanged()


void FileDetailWidget::fileCommentsChanged()
{
  if( !m_measurment )
    return;
  
  vector<string> remarks;
  const QString txt = m_fileComments->toPlainText();
  const QStringList	strings = txt.split ( "\n", QString::SkipEmptyParts );
  for( int i = 0 ; i < strings.length(); ++i )
    remarks.push_back( strings[i].toUtf8().data() );
  m_measurment->set_remarks( remarks );
}//void fileCommentsChanged()


void FileDetailWidget::filenameChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_filename( m_filename->text().toUtf8().data() );
}//void filenameChanged()


void FileDetailWidget::uuidChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_uuid( m_uuid->text().toUtf8().data() );
}//void uuidChanged()


void FileDetailWidget::inspectionChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_inspection( m_inspection->text().toUtf8().data() );
}//void inspectionChanged()


void FileDetailWidget::laneChanged()
{
  if( !m_measurment )
    return;
  
  bool ok;
  int lane = m_lane->text().toLong(&ok);
  if( ok )
  {
    m_measurment->set_lane_number( lane );
  }else
  {
    char buff[64];
    if( m_measurment->lane_number() > 0 )
      snprintf( buff, sizeof(buff), "%i", m_measurment->lane_number() );
    else
      buff[0] = '\0';
    m_lane->setText( buff );
  }
}//void laneChanged()


void FileDetailWidget::locationChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_measurement_location_name( m_location->text().toUtf8().data() );
}//void locationChanged()


void FileDetailWidget::instTypeChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_inspection( m_inspection->text().toUtf8().data() );
}//void instTypeChanged()


void FileDetailWidget::manufacturerChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_manufacturer( m_manufacturer->text().toUtf8().data() );
}//void manufacturerChanged()


void FileDetailWidget::modelChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_instrument_model( m_model->text().toUtf8().data() );
}//void modelChanged()


void FileDetailWidget::serialChanged()
{
  if( !m_measurment )
    return;
  
  m_measurment->set_instrument_id( m_serial->text().toUtf8().data() );
}//void serialChanged()


void FileDetailWidget::realtimeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  bool ok;
  double val = m_realtime->text().toDouble( &ok );
  if( !ok )
  {
    val = m_meas->real_time();
    char buff[64];
    snprintf( buff, sizeof(buff), "%.3f", val );
    m_realtime->setText( buff );
  }else
    m_measurment->set_real_time( val, m_meas );
}//void realtimeChanged()


void FileDetailWidget::livetimeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  bool ok;
  double val = m_livetime->text().toDouble( &ok );
  if( !ok )
  {
    val = m_meas->live_time();
    char buff[64];
    snprintf( buff, sizeof(buff), "%.3f", val );
    m_realtime->setText( buff );
  }else
    m_measurment->set_live_time( val, m_meas );
}//void livetimeChanged()


void FileDetailWidget::latitudeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  
  bool ok;
  double val = m_latitude->text().toDouble( &ok );
  if( !ok )
  {
    val = m_meas->latitude();
    if( m_meas->has_gps_info() )
    {
      char buff[64];
      snprintf( buff, sizeof(buff), "%.7f", val );
      m_latitude->setText( buff );
    }else
      m_latitude->setText( "" );
  }else
    m_measurment->set_position( m_meas->longitude(), val,
                                m_meas->position_time(), m_meas );
}//void latitudeChanged()


void FileDetailWidget::longitudeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  bool ok;
  double val = m_longitude->text().toDouble( &ok );
  if( !ok )
  {
    val = m_meas->longitude();
    if( m_meas->has_gps_info() )
    {
      char buff[64];
      snprintf( buff, sizeof(buff), "%.7f", val );
      m_longitude->setText( buff );
    }else
      m_longitude->setText( "" );
  }else
    m_measurment->set_position( val, m_meas->latitude(),
                               m_meas->position_time(), m_meas );
}//void longitudeChanged()


void FileDetailWidget::dateTimeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  QDateTime dt = m_date->dateTime();
  if( !dt.isValid() )
  {
	dt = posix_to_qt( m_meas->start_time() );
    m_date->setDateTime( dt );
  }else
  {
    boost::posix_time::ptime pt = qt_to_posix( dt );
    m_measurment->set_start_time( pt, m_meas );
  }
}//void dateTimeChanged()


void FileDetailWidget::posDateTimeChanged()
{
  if( !m_measurment || !m_meas )
    return;
  
  QDateTime dt = m_posdate->dateTime();
  if( !dt.isValid() )
  {
    dt = posix_to_qt( m_meas->position_time() );
    m_posdate->setDateTime( dt );
  }else
  {
    boost::posix_time::ptime pt = qt_to_posix( dt );
    m_measurment->set_position( m_meas->longitude(), m_meas->latitude(), pt, m_meas );
  }
}//void posDateTimeChanged()



void FileDetailWidget::updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                                      std::set<int> samplenums,
                                      std::vector<bool> detectors )
{
  m_samplenums.swap( samplenums );
  m_detectors.swap( detectors );
  m_measurment = meas;
  
  emit displayUpdated( meas, samplenums, detectors );

  const bool notempty = !!m_measurment || m_measurment->measurements().empty();
  
  m_filename->setEnabled( notempty );
  m_uuid->setEnabled( notempty );
  m_inspection->setEnabled( notempty );
  m_lane->setEnabled( notempty );
  m_location->setEnabled( notempty );
  m_instType->setEnabled( notempty );
  m_manufacturer->setEnabled( notempty );
  m_model->setEnabled( notempty );
  m_serial->setEnabled( notempty );
  m_memsize->setEnabled( notempty );
  m_fileComments->setEnabled( notempty );
  m_lane->setEnabled( notempty );
  m_numrecords->setEnabled( notempty );
  
  if( !m_measurment )
  {
    m_filename->setText("");
    m_uuid->setText("");
    m_inspection->setText("");
    m_lane->setText("");
    m_location->setText("");
    m_instType->setText("");
    m_manufacturer->setText("");
    m_model->setText("");
    m_serial->setText("");
    m_memsize->setText("");
    m_fileComments->setText("");
    m_numrecords->setText("");
    changeRecord( -9999 );
    
    return;
  }//if( !m_measurment )
  
  char buffer[128];
  
  m_filename->setText( m_measurment->filename().c_str() );
  m_uuid->setText( m_measurment->uuid().c_str() );
  m_inspection->setText( m_measurment->inspection().c_str() );
  
  if( m_measurment->lane_number() >= 0 )
  {
    snprintf( buffer, sizeof(buffer), "%i", m_measurment->lane_number() );
    m_lane->setText( buffer );
  }else
    m_lane->setText( "" );
  
  m_location->setText( m_measurment->measurement_location_name().c_str() );

  m_instType->setText( m_measurment->instrument_type().c_str() );
  m_manufacturer->setText( m_measurment->manufacturer().c_str() );
  m_model->setText( m_measurment->instrument_model().c_str() );
  m_serial->setText( m_measurment->instrument_id().c_str() );
  
  snprintf( buffer, sizeof(buffer), "%i", int(m_measurment->memmorysize()/1024) );
  m_memsize->setText( buffer );
  m_fileComments->setText( "" );
  
  const vector<string> &remarks = m_measurment->remarks();
  for( size_t i = 0; i < remarks.size(); ++i )
    m_fileComments->append( remarks[i].c_str() );
  
  vector< MeasurementConstShrdPtr > m = m_measurment->measurements();
  m_record->setRange( 1, static_cast<int>(m.size()) );
  
  if( samplenums.size() && detectors.size() )
  {
    MeasurementConstShrdPtr record;
    
    foreach( int sample, samplenums )
    {
      
      for( size_t i = 0; i < detectors.size(); ++i )
      {
        if( detectors[i] )
        {
          const int detnum = m_measurment->detector_numbers()[i];
          record = m_measurment->measurement( sample, detnum );
          if( !!record )
            break;
        }
      }//for( size_t i = 0; i < detectors.size(); ++i )
      
      if( !!record )
        break;
    }//foreach( int sample, samplenums )
    
    if( !record )
    {
      changeRecord( 1 );
    }else
    {
      vector< MeasurementConstShrdPtr >::const_iterator pos;
      pos = std::find( m.begin(), m.end(), record );
      if( pos == m.end() )
        changeRecord( 1 );
      else
        changeRecord( static_cast<int>(1 + (pos-m.begin())) );
    }//if( !record ) / else
  }else
  {
    changeRecord( 1 );
  }
  
  snprintf( buffer, sizeof(buffer), "of %i",
            int(m_measurment->measurements().size()) );
  m_numrecords->setText( buffer );

}//void updateDisplay(...)



void FileDetailWidget::combineChannels( const int ncombine, const bool all )
{
  if( !m_measurment )
  {
    cerr << "FileDetailWidget::combineChannels(): invalid spectrum file ptr"
         << endl;
    return;
  }//if( !m_measurment || !m_meas )
  
  const size_t nchannel = m_meas->num_gamma_channels();
  
  if( all )
    m_measurment->combine_gamma_channels( ncombine, nchannel );
  else if( !!m_meas )
    m_measurment->combine_gamma_channels( ncombine, m_meas );
  else
  {
    cerr << "FileDetailWidget::combineChannels(): invalid Measurment ptr"
         << endl;
    return;
  }
  
  
  emit fileDataModified();
}//void combineChannels( int nchannels )


void FileDetailWidget::createCombineChannelDialog()
{
  new CombineChannelsDialog( this );
}//void createCombineChannelDialog()


void FileDetailWidget::createCropChannelDialog()
{
  new TruncateChannelsDialog( this );
}//void createCropChannelDialog();


void FileDetailWidget::cropToChannels( int first, int last, const bool all )
{
  cerr << "FileDetailWidget::cropToChannels(): " << first << ", " << last << endl;
  const int nchannel = !!m_meas ? int(m_meas->num_gamma_channels()) : int(0);
  if( !nchannel || (first<0 && last<0) )
    return;

  if( first > last )
    std::swap( first, last );

  if( first < 0 )
    first = 0;
  if( last >= nchannel )
    last = nchannel - 1;
  
  if( first==0 && (last+1)==nchannel )
    return;
  
  if( all )
  {
    cerr << "Caling truncate_gamma_channels for all measurment only" << endl;
    const set<size_t> gammacounts = m_measurment->gamma_channel_counts();
    for( set<size_t>::const_iterator i = gammacounts.begin();
         i != gammacounts.end(); ++i )
    {
      if( *i )
      {
        size_t thislast = static_cast<size_t>( last );
        if( thislast >= (*i) )
          thislast = (*i) - 1;
        m_measurment->truncate_gamma_channels( first, thislast, *i, false );
      }
    }
  }else
  {
    cerr << "Caling truncate_gamma_channels for current measurment only" << endl;
    m_measurment->truncate_gamma_channels( first, last, false, m_meas );
  }
  
  emit fileDataModified();
}//void cropToChannels( const int first_channel, const int last_channel )


void FileDetailWidget::energyCalUpdated( std::shared_ptr<MeasurementInfo> meas )
{
  if( meas != m_measurment )
  {
    //..
  }

  emit fileDataModified();
}


void FileDetailWidget::createEnergyCalDialog()
{
  EnergyCalDialog *dialog = new EnergyCalDialog( this );

  QObject::connect( this, SIGNAL(displayUpdated(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)),
                    dialog, SLOT(updateDisplay(std::shared_ptr<MeasurementInfo>,std::set<int>,std::vector<bool>)) );

  emit energyCalDialogCreated();
}//void createEnergyCalDialog()


