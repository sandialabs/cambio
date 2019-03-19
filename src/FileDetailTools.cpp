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


#include <set>
#include <vector>

#include <QLabel>
#include <QDebug>
#include <QDialog>
#include <QWidget>
#include <QLineEdit>
#include <QGroupBox>
#include <QValidator>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSignalMapper>
#include <QDoubleSpinBox>

#define BOOST_DATE_TIME_NO_LIB
#include <boost/date_time/posix_time/posix_time.hpp>

#include "cambio/FileDetailWidget.h"
#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"


#include "cambio/FileDetailTools.h"


using namespace std;



class CombineChannelValidator : public QValidator
{
  const int m_nchannel;
public:
  CombineChannelValidator( const int nchannel, QObject *parent = 0 )
  : QValidator( parent ),
  m_nchannel( nchannel )
  {
  }
  //  virtual void fixup( QString &input ) const {}
  virtual QValidator::State	validate( QString &input, int & ) const
  {
    bool converted;
    const int value = input.toInt( &converted );
    if( !converted || value < 2 || ((m_nchannel % value) != 0) )
      return QValidator::Intermediate;
    return (value <= m_nchannel) ? QValidator::Acceptable : QValidator::Invalid;
  }//State validate(...)
};//class CombineChannelValidator




CombineChannelsDialog::CombineChannelsDialog( FileDetailWidget *parent )
: QDialog( parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint ),
m_nchannel( 0 ),
m_status( 0 ),
m_ncombine( 0 ),
m_applyToGroup( 0 ),
m_accept( 0 ),
m_parent( parent )
{
  assert( parent );
  
  if( !m_parent->m_meas || !m_parent->m_measurment )
  {
    deleteLater();
    return;
  }//if( !m_parent->m_meas )
  
  m_nchannel = static_cast<int>( m_parent->m_meas->num_gamma_channels() );
  
  if( m_nchannel < 2 )
  {
    deleteLater();
    return;
  }//if( m_nchannel < 2 )
  
  setWindowTitle( "Channel Edit" );
  setModal( true );
  
  QGridLayout *layout = new QGridLayout();
  setLayout( layout );
  
  QLabel *label = new QLabel( "Number of channels to combine: " );
  layout->addWidget( label, 0, 0 );
  
  m_ncombine = new QLineEdit();
  layout->addWidget( m_ncombine, 0, 1 );
  QObject::connect( m_ncombine, SIGNAL(textEdited(const QString &)), this, SLOT(checkNumChannelsValidity()) );
  m_ncombine->setInputMask( "0000" );
  
  CombineChannelValidator *validator = new CombineChannelValidator( m_nchannel, m_ncombine );
  m_ncombine->setValidator( validator );
  
  m_status = new QLabel();
  layout->addWidget( m_status, layout->rowCount(), 0, 1, 2 );
  
  const vector< std::shared_ptr<const Measurement> > measurements
  = m_parent->m_measurment->measurements();
  const size_t nmeas = measurements.size();
  const size_t ngammachan = m_parent->m_meas->num_gamma_channels();
  
  bool multipleMeas = ((nmeas > 1) && (ngammachan > 0));
  if( multipleMeas )
  {
    for( size_t i = 0; multipleMeas && (i < nmeas); ++i )
    {
      const size_t thisnchannel = measurements[i]->num_gamma_channels();
      multipleMeas = (!thisnchannel || (thisnchannel == ngammachan));
    }//for( size_t i = 0; i < nmeas; ++i )
  }//if( multipleMeas )
  
  
  if( multipleMeas )
  {
    QWidget *spacer = new QWidget();
    spacer->resize( 13, 13 );
    layout->addWidget( spacer, layout->rowCount(), 0, 1, 2 );
    
    QGroupBox *box = new QGroupBox();
    QGridLayout *boxlayout = new QGridLayout();
    box->setLayout( boxlayout );
    box->setTitle( "Apply To:" );
    layout->addWidget( box, layout->rowCount(), 0, 1, 2 );
    
    m_applyToGroup = new QButtonGroup( box );
    
    std::vector< MeasurementConstShrdPtr > meass
                                       = m_parent->m_measurment->measurements();
    
    std::vector< MeasurementConstShrdPtr >::const_iterator pos
                    = std::find( meass.begin(), meass.end(), m_parent->m_meas );
    const int record = 1 + static_cast<int>( pos - meass.begin() );
    
    char buff[128];
    snprintf( buff, sizeof(buff), "Record %i only", record );
    
    QRadioButton *button = new QRadioButton( buff );
    m_applyToGroup->addButton( button, 0 );
    boxlayout->addWidget( button, 0, 0, Qt::AlignJustify );
    
    button = new QRadioButton( "All Records" );
    m_applyToGroup->addButton( button, 1 );
    boxlayout->addWidget( button, 0, 1, Qt::AlignJustify );
    button->setChecked( true );
  }//if( there are multiple measurments this could apply to )
  
  
  QWidget *buttons = new QWidget();
  layout->addWidget( buttons, layout->rowCount(), 0, 1, 2 );
  
  QGridLayout *buttonLayout = new QGridLayout();
  buttonLayout->setContentsMargins( 1,1,1,1 );
  buttons->setLayout( buttonLayout );
  
  m_accept = new QPushButton( "Accept" );
  buttonLayout->addWidget( m_accept, 0, 0, Qt::AlignJustify );
  QObject::connect( m_accept, SIGNAL(clicked(bool)), this, SLOT(doChanges()) );
  
  QPushButton *cancel = new QPushButton( "Cancel" );
  QObject::connect( cancel, SIGNAL(clicked(bool)), this, SLOT(close()) );
  buttonLayout->addWidget( cancel, 0, 1, Qt::AlignJustify );
  
  QObject::connect( this, SIGNAL(rejected()), this, SLOT(deleteLater()) );
  
  m_accept->setEnabled( false );
  
  checkNumChannelsValidity();
  
  show();
  raise();
  activateWindow();
}//CombineChannelsDialog constructor


CombineChannelsDialog::~CombineChannelsDialog()
{
}


void CombineChannelsDialog::closeEvent( QCloseEvent *event )
{
  QDialog::closeEvent( event );
  deleteLater();
}


void CombineChannelsDialog::checkNumChannelsValidity()
{
  if( !m_nchannel || ! m_accept || !m_ncombine || !m_ncombine->validator() )
    return;
  
  if( !m_parent->m_meas )
  {
    deleteLater();
    return;
  }//if( !m_parent->m_meas )
  
  m_status->setText( "---" );
  
  int pos = 0;
  QString txt = m_ncombine->text();
  const QValidator *val = m_ncombine->validator();
  const bool valid = (val && (val->validate(txt,pos) == QValidator::Acceptable));
  m_accept->setEnabled( valid );
  
  char buff[256];
  
  if( valid )
  {
    const int userval = txt.toInt();
    const int nchan = m_nchannel / userval;
    snprintf( buff, sizeof(buff), "Will reduce from %i to %i channels",
             m_nchannel, nchan );
    m_status->setText( buff );
  }else
  {
    if( txt.size() < 1 )
    {
      snprintf( buff, sizeof(buff), "Enter a number from 2 to %i that divides"
               " %i evenly.", m_nchannel, m_nchannel );
      m_status->setText( buff );
      return;
    }
    
    bool convertionOk;
    const int nchannel = txt.toInt( &convertionOk );
    
    if( !convertionOk )
    {
      m_status->setText( "Invalid Input" );
      return;
    }
    
    if( nchannel < 2 )
    {
      m_status->setText( "You must enter a integer larger than one." );
      return;
    }
    
    if( nchannel > m_nchannel )
    {
      snprintf( buff, sizeof(buff),
               "You must enter a integer less than or equal to %i.",
               m_nchannel );
      m_status->setText( buff );
      return;
    }
    
    if( ((m_nchannel % nchannel) != 0) )
    {
      snprintf( buff, sizeof(buff),
               "The number must evenly divide %i", m_nchannel );
      m_status->setText( buff );
      return;
    }
  }//if( valid ) / else
}//void checkNumChannelsValidity()


void CombineChannelsDialog::doChanges()
{
  if( !m_parent->m_meas )
  {
    deleteLater();
    return;
  }//if( !m_parent->m_meas )
  
  int pos = 0;
  QString txt = m_ncombine->text();
  const QValidator *val = m_ncombine->validator();
  const bool valid = (val && (val->validate(txt,pos) == QValidator::Acceptable));
  
  if( !valid )
    return;
  
  const bool all = (m_applyToGroup && (m_applyToGroup->checkedId() == 1));
  
  const int nchannel = m_ncombine->text().toInt();
  m_parent->combineChannels( nchannel, all );
  
  close();
}//void doChanges()





//------------       TruncateChannelsDialog            -----------------------//
TruncateChannelsDialog::TruncateChannelsDialog( FileDetailWidget *parent )
  : QDialog( parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint ),
    m_lowerChannel( 0 ),
    m_upperChannel( 0 ),
    m_lowerEnergy( 0 ),
    m_upperEnergy( 0 ),
    m_applyToGroup( 0 ),
    m_accept( 0 ),
    m_parent( parent )
{
  assert( parent );
  
  if( !m_parent->m_meas || !m_parent->m_measurment )
  {
    deleteLater();
    return;
  }//if( !m_parent->m_meas )
  
  const size_t nchannel = m_parent->m_meas->num_gamma_channels();
  
  if( nchannel < 2 )
  {
    deleteLater();
    return;
  }//if( m_nchannel < 2 )
  
  setWindowTitle( "Energy Truncate" );
  setModal( true );
  
  QGridLayout *layout = new QGridLayout();
  setLayout( layout );
  
  QLabel *label = new QLabel( "Channels to keep:" );
  layout->addWidget( label, 0, 0 );
  
  m_lowerChannel = new QSpinBox();
  layout->addWidget( m_lowerChannel, 0, 1 );
  
  label = new QLabel( "to" );
  layout->addWidget( label, 0, 2 );
  
  m_upperChannel = new QSpinBox();
  layout->addWidget( m_upperChannel, 0, 3 );
  
  label = new QLabel( "Energies to keep:" );
  layout->addWidget( label, 1, 0 );
  
  m_lowerEnergy = new QDoubleSpinBox();
  layout->addWidget( m_lowerEnergy, 1, 1 );
  m_lowerEnergy->setSuffix( " keV" );
  
  label = new QLabel( "to" );
  layout->addWidget( label, 1, 2 );
  
  m_upperEnergy = new QDoubleSpinBox();
  layout->addWidget( m_upperEnergy, 1, 3 );
  m_upperEnergy->setSuffix( " keV" );
  
  
  const int nchan = static_cast<int>( m_parent->m_meas->num_gamma_channels() );
  const double lowerenergy = m_parent->m_meas->gamma_channel_lower(0);
  const double upperenergy = m_parent->m_meas->gamma_channel_upper(nchan-1);
  
//  m_lowerChannel->setRange( 0, nchan );
//  m_upperChannel->setRange( 0, nchan );
//  m_lowerEnergy->setRange( lowerenergy, upperenergy );
//  m_upperEnergy->setRange( lowerenergy, upperenergy );
  
  m_lowerChannel->setRange( 0, 131072 );
  m_upperChannel->setRange( 0, 131072 );
  m_lowerEnergy->setRange( -100000.0, 100000.0 );
  m_upperEnergy->setRange( -100000.0, 100000.0 );
  
  m_lowerChannel->setValue( 0 );
  m_upperChannel->setValue( nchan - 1 );
  m_lowerEnergy->setValue( lowerenergy );
  m_upperEnergy->setValue( upperenergy );
  
  QObject::connect( m_lowerChannel, SIGNAL(valueChanged(int)), this, SLOT(channelRangeChanged()) );
  QObject::connect( m_upperChannel, SIGNAL(valueChanged(int)), this, SLOT(channelRangeChanged()) );
  
  QObject::connect( m_lowerEnergy, SIGNAL(valueChanged(double)), this, SLOT(energyRangeChanged()) );
  QObject::connect( m_upperEnergy, SIGNAL(valueChanged(double)), this, SLOT(energyRangeChanged()) );
  
  set<std::shared_ptr<const vector<float> > > xaxiss;
  const vector< std::shared_ptr<const Measurement> > meass
                                       = m_parent->m_measurment->measurements();
  
  //could also test properties_flags_ kHasCommonBinning ...
  for( size_t i = 0; i < meass.size(); ++i )
  {
    const ShrdConstFVecPtr &energies = meass[i]->channel_energies();
    if( energies && energies->size() )
      xaxiss.insert( energies );
  }//for( size_t i = 0; i < measurements.size(); ++i )
  
  
  if( xaxiss.empty() )
  {
    deleteLater();
    return;
  }//if( xaxiss.empty() )
  

  if( xaxiss.size() == 1 && meass.size() > 1 )
  {
    QWidget *spacer = new QWidget();
    spacer->resize( 13, 13 );
    layout->addWidget( spacer, layout->rowCount(), 0, 1, 2 );
    
    QGroupBox *box = new QGroupBox();
    QGridLayout *boxlayout = new QGridLayout();
    box->setLayout( boxlayout );
    box->setTitle( "Apply To:" );
    layout->addWidget( box, layout->rowCount(), 0, 1, 2 );
    
    m_applyToGroup = new QButtonGroup( box );
    
    std::vector< MeasurementConstShrdPtr >::const_iterator pos
                  = std::find( meass.begin(), meass.end(), m_parent->m_meas );
    const int record = 1 + static_cast<int>( pos - meass.begin() );
    
    char buff[128];
    snprintf( buff, sizeof(buff), "Record %i only", record );
    
    QRadioButton *button = new QRadioButton( buff );
    m_applyToGroup->addButton( button, 0 );
    boxlayout->addWidget( button, 0, 0, Qt::AlignJustify );
    
    button = new QRadioButton( "All Records" );
    m_applyToGroup->addButton( button, 1 );
    boxlayout->addWidget( button, 0, 1, Qt::AlignJustify );
    button->setChecked( true );
  }//if( there are multiple measurments this could apply to )
  
  
  QWidget *buttons = new QWidget();
  layout->addWidget( buttons, layout->rowCount(), 0, 1, 2 );
  
  QGridLayout *buttonLayout = new QGridLayout();
  buttonLayout->setContentsMargins( 1,1,1,1 );
  buttons->setLayout( buttonLayout );
  
  m_accept = new QPushButton( "Accept" );
  buttonLayout->addWidget( m_accept, 0, 0, Qt::AlignJustify );
  QObject::connect( m_accept, SIGNAL(clicked(bool)), this, SLOT(doChanges()) );
  
  QPushButton *cancel = new QPushButton( "Cancel" );
  QObject::connect( cancel, SIGNAL(clicked(bool)), this, SLOT(close()) );
  buttonLayout->addWidget( cancel, 0, 1, Qt::AlignJustify );
  
  QObject::connect( this, SIGNAL(rejected()), this, SLOT(deleteLater()) );
  
//  m_accept->setEnabled( false );
  
  show();
  raise();
  activateWindow();
}//CombineChannelsDialog constructor


TruncateChannelsDialog::~TruncateChannelsDialog()
{
  
}


void TruncateChannelsDialog::closeEvent( QCloseEvent *event )
{
  QDialog::closeEvent( event );
  deleteLater();
}


void TruncateChannelsDialog::doChanges()
{
  if( !m_parent->m_meas )
  {
    deleteLater();
    return;
  }//if( !m_parent->m_meas )

  const bool all = (m_applyToGroup && (m_applyToGroup->checkedId() == 1));
  
  const int lowerchannel = m_lowerChannel->value();
  const int upperchannel = m_upperChannel->value();
  m_parent->cropToChannels( lowerchannel, upperchannel, all );
  
  close();
}//void doChanges()


void TruncateChannelsDialog::energyRangeChanged()
{
  std::shared_ptr<const Measurement> m = m_parent->m_meas;
  
  if( !m )
    return;
  
  const int nchan = static_cast<int>( m->num_gamma_channels() );
  const double lowestenergy = m->gamma_channel_lower( 0 );
  const double highestenergy = m->gamma_channel_upper( nchan - 1 );
  
  double lowerenergy = m_lowerEnergy->value();
  double upperenergy = m_upperEnergy->value();
  
  if( lowerenergy < lowestenergy )
    lowerenergy = lowestenergy;
  if( lowerenergy > highestenergy )
    lowerenergy = highestenergy;
  
  if( upperenergy < lowestenergy )
    upperenergy = lowestenergy;
  if( upperenergy > highestenergy )
    upperenergy = highestenergy;
  
  if( lowerenergy > upperenergy )
    std::swap( lowerenergy, upperenergy );
  
  if( fabs(lowerenergy - m_lowerEnergy->value()) > 0.0001 )
    m_lowerEnergy->setValue( lowerenergy );
  
  if( fabs(upperenergy - m_upperEnergy->value()) > 0.0001 )
    m_upperEnergy->setValue( upperenergy );
  
  const size_t lowerchannel = m->find_gamma_channel( float(lowerenergy) );
  const size_t upperchannel = m->find_gamma_channel( float(upperenergy-0.00001) );
  
  const bool oldLowerState = m_lowerChannel->blockSignals(true);
  const bool oldUpperState = m_upperChannel->blockSignals(true);
  
  m_lowerChannel->setValue( int(lowerchannel) );
  m_upperChannel->setValue( int(upperchannel) );
  
  m_lowerChannel->blockSignals( oldLowerState );
  m_upperChannel->blockSignals( oldUpperState );
}//void energyRangeChanged()


void TruncateChannelsDialog::channelRangeChanged()
{
  std::shared_ptr<const Measurement> m = m_parent->m_meas;
  
  if( !m )
    return;
  
  const int nchan = static_cast<int>( m->num_gamma_channels() );
  
  int lowerchannel = m_lowerChannel->value();
  int upperchannel = m_upperChannel->value();
  
  if( lowerchannel < 0 )
    lowerchannel = 0;
  else if( lowerchannel >= nchan )
    lowerchannel = nchan - 1;
  
  if( upperchannel < 0 )
    upperchannel = 0;
  else if( upperchannel >= nchan )
    upperchannel = nchan - 1;
  
  if( lowerchannel > upperchannel )
    std::swap( lowerchannel, upperchannel );
  
  if( lowerchannel != m_lowerChannel->value() )
    m_lowerChannel->setValue( lowerchannel );
  if( upperchannel != m_upperChannel->value() )
    m_upperChannel->setValue( upperchannel );
  
  const float lowerenergy = m->gamma_channel_lower( lowerchannel );
  const float upperenergy = m->gamma_channel_upper( upperchannel );
  
  const bool oldLowerState = m_lowerEnergy->blockSignals(true);
  const bool oldUpperState = m_upperEnergy->blockSignals(true);
  
  m_lowerEnergy->setValue( lowerenergy );
  m_upperEnergy->setValue( upperenergy );
  
  m_lowerEnergy->blockSignals( oldLowerState );
  m_upperEnergy->blockSignals( oldUpperState );
}//void channelRangeChanged()



//------------       EnergyCalDialog       -----------------------//
EnergyCalDialog::EnergyCalDialog( FileDetailWidget *parent )
  : QDialog( parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint ),
    m_original(),
	m_remove_cal( 0 ),
    m_parent( parent )
{
  if( !parent )  //JIC
	return;
	 
  //Special Cases:
  //  1) Single spectrum in file
  //     -easy
  //  2) Single calibration in file, single detector, but multiple spectra
  //     -Offer choice on which spectra to apply calibration to; default to all
  //  3) Multiple detectors, but each with own calibration
  //     -Offer to apply change to either all samples, or selected samples, or 
  //  4) Multiple detectors, each with own calibration, but consistent through file
  //     -
  //  5) Calibrations change throughout file
  //
  //  Maybe implement the selecting of smaple numbers throughout rest of app
  //  and then use


  m_original = parent->m_measurment;
 
  QGridLayout *layout = new QGridLayout();
  layout->setContentsMargins( 1,1,1,1 );
  setLayout( layout );
  
  QLabel *label = new QLabel( "Energy Calibration Under Contruction" );
  layout->addWidget( label, 0, 0, 1, 2 );

  m_remove_cal = new QPushButton( "Remove Calibration" );
  layout->addWidget( m_remove_cal, 1, 0, Qt::AlignJustify );
  QObject::connect( m_remove_cal, SIGNAL(clicked(bool)), this, SLOT(removeCal()) );

  QPushButton *cancel = new QPushButton( "Cancel" );
  QObject::connect( cancel, SIGNAL(clicked(bool)), this, SLOT(close()) );
  layout->addWidget( cancel, 1, 1, Qt::AlignJustify );
 
  show();
  raise();
  activateWindow();
}//EnergyCalDialog constructor
  

EnergyCalDialog::~EnergyCalDialog()
{
}//EnergyCalDialog destructor
  
  
void EnergyCalDialog::removeCal()
{
  std::shared_ptr<MeasurementInfo> meas = m_parent->m_measurment;

  //std::vector< std::shared_ptr<const Measurement> > meass = m_parent->m_measurment->measurements();
  if( !meas )
    return;

  std::vector<float> eqn;
  eqn.push_back( 0.0f );
  eqn.push_back( 1.0f );
  DeviationPairVec dev_pairs;
  meas->recalibrate_by_eqn( eqn, dev_pairs, Measurement::Polynomial );

  m_parent->energyCalUpdated( meas );

  QMessageBox msgBox;
  msgBox.setText("X-axis is now channel number, and not energy (for current file)");
  msgBox.exec();

  emit close();
}//void removeCal()
  
 
void EnergyCalDialog::updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                                      std::set<int> samplenums,
                                      std::vector<bool> detectors )
{
  if( meas != m_original )
  {
  }

}//void updateDisplay(...)


void EnergyCalDialog::closeEvent( QCloseEvent *event )
{
  QDialog::closeEvent( event );
  deleteLater();
}//void closeEvent( QCloseEvent *event )
  
 


