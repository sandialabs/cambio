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

#include <fstream>

#include <QDir>
#include <QList>
#include <QLabel>
#include <QDebug>
#include <QString>
#include <QDialog>
#include <QTreeView>
#include <QCheckBox>
#include <QSettings>
#include <QLineEdit>
#include <QGroupBox>
#include <QFileInfo>
#include <QValidator>
#include <QListWidget>
#include <QMessageBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QApplication>
#include <QDesktopWidget>
#include <QProgressDialog>
#include <QFileSystemModel>

#include <boost/scoped_ptr.hpp>

#include "SpecUtils/SpecFile.h"
#include "SpecUtils/Filesystem.h"
#include "cambio/BatchConvertDialog.h"
#include "SpecUtils/D3SpectrumExport.h"

#ifdef _WIN32
using SpecUtils::convert_from_utf8_to_utf16;
#endif

namespace
{
  const char *description( const SpecUtils::SaveSpectrumAsType type )
  {
    using SpecUtils::SaveSpectrumAsType;
    switch( type )
    {
      case SaveSpectrumAsType::Txt:                return "TXT";
      case SaveSpectrumAsType::Csv:                return "CSV";
      case SaveSpectrumAsType::Pcf:                return "PCF";
      case SaveSpectrumAsType::N42_2006:           return "N42 - 2006";
      case SaveSpectrumAsType::N42_2012:           return "N42 - 2012";
      case SaveSpectrumAsType::Chn:                return "CHN";
      case SaveSpectrumAsType::SpcBinaryInt:       return "INT SPC";
      case SaveSpectrumAsType::SpcBinaryFloat:     return "FLT SPC";
      case SaveSpectrumAsType::SpcAscii:           return "ASCII SPC";
      case SaveSpectrumAsType::ExploraniumGr130v0: return "GR130 DAT";
      case SaveSpectrumAsType::ExploraniumGr135v2: return "GR135 DAT";
      case SaveSpectrumAsType::SpeIaea:            return "IAEA SPE";
      case SaveSpectrumAsType::Cnf:                return "Canberra CNF";
      case SaveSpectrumAsType::Tka:                return "TKA";
#if( SpecUtils_ENABLE_D3_CHART )
      case SaveSpectrumAsType::HtmlD3:             return "HTML";
#endif
      case SaveSpectrumAsType::NumTypes:           return "";
    }
    return "";
  }//const char *descriptionText( const SaveSpectrumAsType type )
  
  bool file_smaller_than( const std::string &path, void *maxsizeptr )
  {
    const size_t *maxsize = (const size_t *)maxsizeptr;
    if( SpecUtils::file_size(path) > (*maxsize) )
      return false;
    
    return true;
  }

  bool maybe_spec_file( const std::string &path, void *maxsizeptr )
  {
    if( SpecUtils::likely_not_spec_file( path ) )
      return false;
    
    if( maxsizeptr )
      return file_smaller_than( path, maxsizeptr );
    
    return true;
  }
}//namespace


class DirectoryValidator : public QValidator
{
public:
 DirectoryValidator( QObject * parent = nullptr )
   : QValidator( parent )
  {
  }
  
  ~DirectoryValidator()
  {
  }
  
  virtual QValidator::State validate( QString &input, int & ) const
  {
    QDir dir( input );
    if( dir.exists() && dir.isReadable() )
      return QValidator::Acceptable;
  
    if( !dir.cdUp() )
      return QValidator::Invalid;
    
    const QList<QString> subs = dir.entryList( (QDir::Dirs|QDir::Readable) );
    
    for( int i = 0; i < subs.size(); ++i )
    {
      QDir thisdir = dir;
      thisdir.cd( subs[i] );
      if( thisdir.path().startsWith(input) )
        return QValidator::Intermediate;
    }
    
    return QValidator::Invalid;
  }//validate(...)
};//class DirectoryValidator


BatchConvertDialog::BatchConvertDialog( QWidget * parent, Qt::WindowFlags f )
  : QDialog( parent, f ),
     m_layout( new QGridLayout() ),
     m_filesystemView( nullptr ),
     m_filesystemModel( nullptr ),
     m_formatBox( nullptr ),
     m_format( nullptr ),
     m_saveToPath( nullptr ),
     m_recursive( nullptr ),
     m_convert( nullptr )
{
  setWindowTitle( "Batch File Conversion" );
  setLayout( m_layout );
  setSizeGripEnabled( true );
  
  QRect screenGeom = QApplication::desktop()->screenGeometry();
  const int height = static_cast<int>( 0.66*screenGeom.height() );
  const int width = static_cast<int>( 0.66*screenGeom.width() );
  resize( std::max(width, 400), std::max(height, 350) );
  
  m_filesystemModel = new QFileSystemModel;
  m_filesystemModel->setRootPath( QDir::homePath() );

  m_filesystemView = new QTreeView();
  m_filesystemView->setModel( m_filesystemModel );
  m_filesystemView->setSelectionMode( QAbstractItemView::MultiSelection );
  m_filesystemView->setColumnHidden( 1, true );
  m_filesystemView->setColumnHidden( 2, true );
  m_filesystemView->setColumnHidden( 3, true );
  
  m_layout->addWidget( m_filesystemView, 0, 0 );
  
  m_formatBox = new QGroupBox( "Format To Export" );
  m_format = new QButtonGroup( this );
  m_format->setExclusive( true );
  QGridLayout *formatlayout = new QGridLayout;
  m_formatBox->setLayout( formatlayout );
  
  for( SpecUtils::SaveSpectrumAsType i = SpecUtils::SaveSpectrumAsType(0);
      i < SpecUtils::SaveSpectrumAsType::NumTypes;
      i = SpecUtils::SaveSpectrumAsType(static_cast<int>(i)+1) )
  {
    QRadioButton *radio = new QRadioButton( description(i) );
    m_format->addButton( radio, static_cast<int>(i) );
    formatlayout->addWidget( radio, static_cast<int>(i), 0 );
  }//for(...)
  
  m_format->button(static_cast<int>(SpecUtils::SaveSpectrumAsType::N42_2012))->setChecked( true );
  
  m_layout->addWidget( m_formatBox, 0, 1, Qt::AlignVCenter );
  
  //Could add a "Consolidate All" options here.
  //  -Spectra ordering by file name, measurment time, file modification date, etc.
  //  -Could add option to have title of spectra be same as input file, or filename
  
  
  QWidget *holder = new QWidget;
  QGridLayout *holderlayout = new QGridLayout;
  holder->setLayout( holderlayout );
  
  QLabel *label = new QLabel( "Output Location:" );
  m_saveToPath = new QLineEdit;
  m_saveToPath->setValidator( new DirectoryValidator( m_saveToPath ) );
  

  QSettings settings;
  QVariant var = settings.value( "SaveFileDir" );
  const QString val = var.isValid() ? var.toString() : QString("");
  if( val.size()>2 && QDir(val).exists() )
	m_saveToPath->setText( val );


  QPushButton *browse = new QPushButton( "Browse..." );
  holderlayout->addWidget( label, 0, 0 );
  holderlayout->addWidget( m_saveToPath, 0, 1 );
  holderlayout->addWidget( browse, 0, 2 );
  holderlayout->setColumnStretch( 1, 10 );
  
  m_layout->addWidget( holder, 1, 0, 1, 2 );
  
  m_layout->setRowStretch( 0, 10 );
  
  
  holder = new QWidget;
  holderlayout = new QGridLayout;
  holder->setLayout( holderlayout );
  
  m_recursive = new QCheckBox( "recursive" );
  m_recursive->setChecked( false );
  m_recursive->setToolTip( "Recursively add files for any selected directory." );
  m_recursive->setHidden( true );
  holderlayout->addWidget( m_recursive, 0, 0 );
  holderlayout->addWidget( new QWidget(), 0, 1 );

  QPushButton *cancel = new QPushButton( "Close" );
  m_convert = new QPushButton( "Convert" );
//  m_convert->setEnabled( false );
  holderlayout->addWidget( cancel, 0, 2 );
  holderlayout->addWidget( m_convert, 0, 3 );
  holderlayout->setColumnStretch( 1, 10 );
  
  m_layout->addWidget( holder, 2, 0, 1, 2 );
  
  
  QObject::connect( cancel, SIGNAL(pressed()), this, SLOT(reject()) );
  QObject::connect( browse, SIGNAL(pressed()), this, SLOT(browseForDirectory()) );
  QObject::connect( m_convert, SIGNAL(pressed()), this, SLOT(convert()) );
}//BatchConvertDialog constructor


BatchConvertDialog::~BatchConvertDialog()
{
  qDebug() << "~BatchConvertDialog()";
}//~BatchConvertDialog()


void BatchConvertDialog::browseForDirectory()
{
  const QString caption = "Select directory to save results to";
  const QString initialDir = m_saveToPath->text();
  QFileDialog dialog( this, caption, initialDir );
  dialog.setFileMode( QFileDialog::DirectoryOnly );
  dialog.setViewMode( QFileDialog::Detail );
  
  const int status = dialog.exec();
  
  if( status != QDialog::Accepted )
    return;
  
  QStringList selecteddirs = dialog.selectedFiles();
  if( selecteddirs.length() > 0 )
  {
	QSettings settings;
    if( QDir(selecteddirs[0]).exists() )
      settings.setValue( "SaveFileDir", selecteddirs[0] );

    m_saveToPath->setText( selecteddirs[0] );
  }
}//void browseForDirectory()

  
void BatchConvertDialog::setDirectory( const QString &path )
{
  QDir dir( path );
  if( !dir.isAbsolute() )
	dir = dir.absolutePath();

  //See if there are any directories 
  QFileInfoList subdirinfo = dir.entryInfoList( (QDir::Dirs|QDir::Readable) );
  qDebug() << "There are " << subdirinfo.size() << " subdirs";
  if( subdirinfo.size() > 2 )
	m_recursive->setVisible( true );

  const bool recursive = m_recursive->isChecked();
  const bool extfilter = true;
  const size_t maxsize = 250*1024*1024;  //250 MByte, chosen arbitrarily.
  SpecUtils::file_match_function_t filterfcn = extfilter ? &maybe_spec_file : &file_smaller_than;
   
  const std::string srcdir = dir.absolutePath().toUtf8().data();
  std::vector<std::string> files;
  if( recursive )
    files = SpecUtils::recursive_ls( srcdir, filterfcn, (void *)&maxsize );
  else
    files = SpecUtils::ls_files_in_directory( srcdir, filterfcn, (void *)&maxsize );
 
  if( files.size() == 0 && !recursive )
  {
    files = SpecUtils::recursive_ls( srcdir, filterfcn, (void *)&maxsize );
	m_recursive->setChecked( !files.empty() );
  }

  qDebug() << "There are " << files.size() << " files";

  //If there are to many file to highlight it seems really slow to update the gui...
  if( files.size() < 20 )
  {
    QStringList entries;
    for( size_t i = 0; i < files.size(); ++i )
      entries.push_back( QString(files[i].c_str()) );
    setFiles( entries );
  }else
  {
    QItemSelectionModel *selectionmodel = m_filesystemView->selectionModel();
    Q_ASSERT( selectionmodel );
    selectionmodel->clearSelection();
   
	 //XXX - need to collapse all nodes!

	QModelIndex index = m_filesystemModel->index ( dir.absolutePath() );
    selectionmodel->select( index, QItemSelectionModel::Select );
    m_filesystemView->scrollTo( index, QAbstractItemView::EnsureVisible );
  }
}//void setDirectory( const QString &path )


void BatchConvertDialog::setFiles( const QList<QString> &files )
{
  QItemSelectionModel *selectionmodel = m_filesystemView->selectionModel();
  
  Q_ASSERT( selectionmodel );
  
  selectionmodel->clearSelection();
  
  for( int i = 0; i < files.size(); ++i )
  {
    QModelIndex index = m_filesystemModel->index ( files[i] );
    selectionmodel->select( index, QItemSelectionModel::Select );
    m_filesystemView->scrollTo( index, QAbstractItemView::EnsureVisible );
  }
  
  QDir savedir( m_saveToPath->text() );
  if( files.size() && (!savedir.exists() || !savedir.isReadable()) )
  {
    QFileInfo info( files[files.size()-1] );
    m_saveToPath->setText( info.path() );
  }//if( files.size() )
}//void setFiles( const QList<QString> &files )


void BatchConvertDialog::convert()
{
  QDir savetodir( m_saveToPath->text() );
  
  if( !savetodir.isReadable() || !savetodir.exists() )
  {
    QMessageBox msgBox;
    msgBox.setText( "Save to path is invalid; please select a writable directory." );
    msgBox.exec();
    return;
  }//if( !savetodir.isReadable() || !savetodir.exists() )
  

  //Put up a busy busy indicator
  QProgressDialog progress( "Gathering files to convert...", "Abort Convert", 0, 1, this );
  progress.setWindowModality(Qt::WindowModal);
  progress.show();

  qApp->processEvents();
  
  QItemSelectionModel *selectionmodel = m_filesystemView->selectionModel();
  Q_ASSERT( selectionmodel );
  QModelIndexList selected = selectionmodel->selectedIndexes();
  
  QList<QString> selectedfiles;
  for( int i = 0; i < selected.size(); ++i )
  {
    QFileInfo info( m_filesystemModel->filePath( selected[i] ) );
    if( info.isFile() && info.isReadable() )
      selectedfiles.push_back( info.absoluteFilePath() );
	else if( info.isDir() && info.isReadable() )
	{
	  const bool recursive = m_recursive->isChecked();
      const bool extfilter = true;
      const size_t maxsize = 250*1024*1024;  //250 MByte, chosen arbitrarily.
      SpecUtils::file_match_function_t filterfcn = extfilter ? &maybe_spec_file : &file_smaller_than;
   
      const std::string srcdir = info.absolutePath().toUtf8().data();
      std::vector<std::string> files;
      if( recursive )
        files = SpecUtils::recursive_ls( srcdir, filterfcn, (void *)&maxsize );
      else
        files = SpecUtils::ls_files_in_directory( srcdir, filterfcn, (void *)&maxsize );
	  
	  for( size_t j = 0; j < files.size(); ++j )
		selectedfiles.push_back( QString(files[j].c_str()) );
	}
  }//for( int i = 0; i < selected.size(); ++i )


  if( selectedfiles.empty() || progress.wasCanceled() )
  {
    QMessageBox msgBox;
	const char * const txt = (selectedfiles.empty() ? "No files are selected." : "Operation Canceled.");
    msgBox.setText( txt );
    msgBox.exec();
    return;
  }//if( selectedfiles.empty() )
 
  progress.setLabelText( "Converting Files..." );
  progress.setMaximum( static_cast<int>( selectedfiles.size() ) );

  const SpecUtils::SaveSpectrumAsType type = SpecUtils::SaveSpectrumAsType(m_format->checkedId());
  
  bool overwriteall = false, skipexisting = false;
  
  QList<QString> converted, failed;
  int i;
  for( i = 0; i < selectedfiles.size(); ++i )
  {
    const QString &path = selectedfiles[i];
    QFileInfo input( path );
    QFileInfo outputInfo(savetodir, input.fileName());
    QString out = outputInfo.absoluteFilePath();
    
    
    const int pos = out.lastIndexOf( '.' );
    if( pos > 0 )
      out = out.left( pos );
      
    
    out += ".";
    out += SpecUtils::suggestedNameEnding(type);
    
    outputInfo = QFileInfo( out );

    SpecUtils::SpecFile meas;
    const bool opened = meas.load_file( path.toUtf8().data(), SpecUtils::ParserType::Auto );
    
    if( !opened )
    {
      failed.push_back( "Couldnt open file: '" + QFileInfo(path).fileName() + "' as a spectrum file." );
      continue;
    }
    
    meas.set_filename( SpecUtils::filename( meas.filename() ) );
    
    //XXX - doesnt properly account for CHN files
    if( skipexisting && outputInfo.exists() )
      continue;
        
    if( !overwriteall && outputInfo.exists() )
    {
      QMessageBox msg;
      msg.setWindowTitle( "File Exists" );
      
      msg.setText( "File: '" + outputInfo.absoluteFilePath()
                   + "' already exists.\n"
                    "Overwrite?" );
      msg.setStandardButtons( QMessageBox::Yes | QMessageBox::YesToAll
                                | QMessageBox::No | QMessageBox::NoToAll );
        
      const int status = msg.exec();
      
      if( status == QMessageBox::No )
        continue;
      
      if( status == QMessageBox::YesToAll )
        overwriteall = true;
      
      if( status == QMessageBox::NoToAll )
      {
        skipexisting = true;
        continue;
      }
    }//if( !overwriteall && outputInfo.exists() )
    
    const std::string outname = out.toUtf8().data();
    boost::scoped_ptr<std::ofstream> output;
    
    if( type != SpecUtils::SaveSpectrumAsType::Chn
        && type != SpecUtils::SaveSpectrumAsType::SpcBinaryFloat
        && type != SpecUtils::SaveSpectrumAsType::SpcBinaryInt )
    {
#ifdef _WIN32
      output.reset( new std::ofstream( convert_from_utf8_to_utf16(outname).c_str(), std::ios::binary | std::ios::out ) );
#else
      output.reset( new std::ofstream( outname.c_str(), std::ios::binary | std::ios::out ) );
#endif
      
      
      if( !output->is_open() )
      {
        failed.push_back( "Couldnt open '" + out + "' for writing" );
        continue;
      }//if( !output.is_open() )
    }//if( type is CHN or SPC )
    
    
    bool ok = true;
    switch( type )
    {
      case SpecUtils::SaveSpectrumAsType::Txt:
        ok = meas.write_txt( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::Csv:
        ok = meas.write_csv( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::Pcf:
        ok = meas.write_pcf( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::N42_2006:
        ok = meas.write_2006_N42( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::N42_2012:
        ok = meas.write_2012_N42( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::ExploraniumGr130v0:
        ok = meas.write_binary_exploranium_gr130v0( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::ExploraniumGr135v2:
        ok = meas.write_binary_exploranium_gr135v2( *output );
        break;
        
      case SpecUtils::SaveSpectrumAsType::Chn:
      case SpecUtils::SaveSpectrumAsType::SpcBinaryInt:
      case SpecUtils::SaveSpectrumAsType::SpcBinaryFloat:
      case SpecUtils::SaveSpectrumAsType::SpcAscii:
      case SpecUtils::SaveSpectrumAsType::SpeIaea:
      case SpecUtils::SaveSpectrumAsType::Cnf:
      case SpecUtils::SaveSpectrumAsType::Tka:
#if( SpecUtils_ENABLE_D3_CHART )
      case SpecUtils::SaveSpectrumAsType::HtmlD3:
#endif
      {
        int nwroteone = 0;
        const std::set<int> samplenums = meas.sample_numbers();
        const std::vector<int> detnums = meas.detector_numbers();
        const std::vector<std::string> detnames = meas.detector_names();
        assert( detnums.size() == detnames.size() );
        
        foreach( const int sample, samplenums )
        {
          foreach( const int detnum, detnums )
          {
            std::set<int> detnumset, samplenumset;
            detnumset.insert( detnum );
            samplenumset.insert( sample );
            
            const auto detnumpos = std::find(std::begin(detnums), std::end(detnums), detnum)
                                   - std::begin(detnums);
            std::vector<std::string> detname( 1, detnames[detnumpos] );
            
            
            QString extention;
            QString outname = out;
            
            const int pos = outname.lastIndexOf( '.' );
            if( pos >= 0 )
            {
              extention = outname.right( outname.size() - pos );
              outname = outname.left( pos );
            }//if( pos >= 0 )
            
            char buffer[32];
            snprintf( buffer, sizeof(buffer), "_%i", nwroteone );
            outname += buffer;
            if( extention.size() > 0 )
              outname = outname + extention;
            
#ifdef _WIN32
            std::ofstream output( convert_from_utf8_to_utf16(outname.toUtf8().data()).c_str(),
                                 std::ios::binary | std::ios::out );
#else
            std::ofstream output( outname.toUtf8().data(),
                                 std::ios::binary | std::ios::out );
#endif
            
            if( !output.is_open() )
            {
              failed.push_back( "Couldnt open '" + outname + "' for writing" );
            }else
            {
              bool wrote = false;
              if( type == SpecUtils::SaveSpectrumAsType::Chn )
                wrote = meas.write_integer_chn( output, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::SpcBinaryInt )
                wrote = meas.write_binary_spc( output, SpecUtils::SpecFile::IntegerSpcType, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::SpcBinaryFloat )
                wrote = meas.write_binary_spc( output, SpecUtils::SpecFile::FloatSpcType, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::SpcAscii )
                wrote = meas.write_ascii_spc( output, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::SpeIaea )
                wrote = meas.write_iaea_spe( output, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::Cnf )
                wrote = meas.write_cnf( output, samplenumset, detnumset );
              else if( type == SpecUtils::SaveSpectrumAsType::Tka )
                wrote = meas.write_tka( output, samplenumset, detnumset );
#if( SpecUtils_ENABLE_D3_CHART )
              else if( type == SpecUtils::SaveSpectrumAsType::HtmlD3 )
                  wrote = meas.write_d3_html( output, D3SpectrumExport::D3SpectrumChartOptions{}, samplenumset, detname );
#endif
              else
                assert( 0 );
            
              
              if( !wrote )
                failed.push_back( "Possibly failed writing '" + outname + "'" );
              else
                converted.push_back( outname );
              
              nwroteone += wrote;
            }
          }//foreach( const int detnum, detnums )
        }//foreach( const int sample, samplenums )
        break;
      }//case kChnSpectrumFile, or SPC
        
      case SpecUtils::SaveSpectrumAsType::NumTypes:
                  break;
    }//switch( type )
    
    if( !ok )
      failed.push_back( "Possibly failed writing '" + out + "'" );
    else
      converted.push_back( out );

	if( progress.wasCanceled() )
	  break;

	progress.setValue( i );
	qApp->processEvents();
  }//for( int i = 0; i < selectedfiles.size(); ++i )
  
  
  QDialog results( this );
  QGridLayout *resultLayout = new QGridLayout;
  results.setLayout( resultLayout );
  QLabel *label = new QLabel();
  resultLayout->addWidget( label, 0, 0 );
  
  char buffer[256];
  
  if( failed.size() )
  {
	if( progress.wasCanceled() )
	{
      snprintf( buffer, sizeof(buffer),
                "Conversion was canceled after %i files, but %i were written, and %i failed",
                i, converted.size(), failed.size() );
	}else
	{
	  snprintf( buffer, sizeof(buffer),
                "Wrote %i files successfully, but failed converting %i input files",
                converted.size(), failed.size() );
	}

    QListWidget *view = new QListWidget;
    
    resultLayout->addWidget( new QLabel("Failed output files"), 1, 0 );
    resultLayout->addWidget( view, 2, 0 );
    resultLayout->setRowStretch( 2, 10 );
    for( int i = 0; i < failed.size(); ++i )
      view->addItem( failed[i] );
    view->setSelectionMode( QAbstractItemView::NoSelection );
  }else if( progress.wasCanceled() )
  {
	snprintf( buffer, sizeof(buffer), "Conversion was canceled, after %i files.",
              converted.size() );
  }else
  {
    snprintf( buffer, sizeof(buffer), "Wrote %i files successfully.",
              converted.size() );
  }
  
  label->setText( buffer );
  
  QPushButton *ok = new QPushButton( "Okay" );
  resultLayout->addWidget( ok, resultLayout->rowCount(), Qt::AlignRight );

  QObject::connect( ok, SIGNAL(pressed()), &results, SLOT(accept()) );
  

  results.exec();
}//void convert();


