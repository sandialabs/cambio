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
#include <QFont>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QDebug>
#include <QWidget>
#include <QDialog>
#include <QSettings>
#include <QFileInfo>
#include <QGroupBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QMessageBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QApplication>
#include <QDesktopWidget>
#include <QStandardPaths>

#include <boost/scoped_ptr.hpp>

#include "cambio/CambioApp.h"
#include "cambio/SaveWidget.h"
#include "cambio/MainWindow.h"
#include "cambio/BusyIndicator.h"
#include "cambio/BatchConvertDialog.h"
#include "SpecUtils/D3SpectrumExport.h"
#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"

using namespace std;

namespace
{
  
const char *description( const SaveSpectrumAsType type )
{
  switch( type )
  {
    case kTxtSpectrumFile:                return "TXT";
    case kCsvSpectrumFile:                return "CSV";
    case kPcfSpectrumFile:                return "PCF";
    case kXmlSpectrumFile:                return "N42 - 2006";
    case k2012N42SpectrumFile:            return "N42 - 2012";
    case kChnSpectrumFile:                return "CHN";
    case kBinaryIntSpcSpectrumFile:       return "SPC - Int";
    case kBinaryFloatSpcSpectrumFile:     return "SPC - Real";
    case kAsciiSpcSpectrumFile:           return "SPC - ASCII";
    case kExploraniumGr130v0SpectrumFile: return "GR130 DAT";
    case kExploraniumGr135v2SpectrumFile: return "GR135 DAT";
    case kIaeaSpeSpectrumFile:            return "SPE IAEA";
#if( SpecUtils_ENABLE_D3_CHART )
    case kD3HtmlSpectrumFile:             return "HTML";
#endif
    case kNumSaveSpectrumAsType:      return "";
  }
  return "";
}//const char *descriptionText( const SaveSpectrumAsType type )

#if( SpecUtils_ENABLE_D3_CHART )
bool writeIndividualHtmlSpectraToOutputFile( std::ofstream &output,
                                              const MeasurementInfo &meas,
                                              const std::set<int> samplenums,
                                              std::vector<bool> detectors )
{
  set<int> detnums;
  const std::vector<int> &detnumsvect = meas.detector_numbers();
  detnums.insert( detnumsvect.begin(), detnumsvect.end() );

//#warning "Saving to HTML files could use some work.\nIf its a passthrough file and all spectra are being saved, and theres a background, then it should be a seperate line.  Similar thing for other files"
  
  const char *endline = "\r\n";
  
  D3SpectrumExport::write_html_page_header( output, meas.filename() );
  
  output << "<body>" << endline;
  
  string page_title = meas.filename();
  
  if( samplenums.size() > 0 )
    output << "<h1>" << page_title << "</h1>" << endline;
  
  for( const int sample : samplenums )
  {
    const string div_id = "chart_sample_" + std::to_string( sample );
    
    string title = meas.filename();
    if( meas.sample_numbers().size() > 1 )
      title += " Sample " + std::to_string(sample);
    
    output << "<div id=\"" << div_id << "\" class=\"chart\" oncontextmenu=\"return false;\";></div>" << endline;
    
    output << "<script>" << endline;
    D3SpectrumExport::write_js_for_chart( output, div_id, title, "Energy (keV)", "Counts" );
    
    auto m = meas.sum_measurements( set<int>{sample}, detectors );
    
    if( !m )
      continue;
    
    std::vector< std::pair<const Measurement *,D3SpectrumExport::D3SpectrumOptions> > measurements;
    measurements.emplace_back( m.get(), D3SpectrumExport::D3SpectrumOptions{} );
    
    D3SpectrumExport::write_and_set_data_for_chart( output, div_id, measurements );
    
    output << "window.addEventListener('resize',function(){spec_chart_" << div_id << ".handleResize();});" << endline;
    
    D3SpectrumExport::D3SpectrumChartOptions options;
    D3SpectrumExport::write_set_options_for_chart( output, div_id, options );
    
    //todo, get rid of this next couple lines
    output << "spec_chart_" << div_id << ".setShowPeaks(1,false);" << endline;
    output << "spec_chart_" << div_id << ".setShowPeaks(2,false);" << endline;
    
    output << "</script>" << endline;
    
    //D3SpectrumExport::write_html_display_options_for_chart( output, div_id, options );
    {//Begin manual write of options
      // Set up option for y-axis scale
      output << "<div style=\"margin-top: 10px; display: inline-block;\"><label>" << endline
      << "Y Scale:" << endline
      << "<select onchange=\"onyscalechange(this,spec_chart_" << div_id << ")\" >" << endline
      << "<option value=\"lin\" " << (!options.m_useLogYAxis ? "selected" : "") << ">Linear</option>" << endline
      << "<option value=\"log\" " << (options.m_useLogYAxis ?  "selected" : "") << ">Log</option>" << endline
      << "<option value=\"sqrt\">Sqrt</option>" << endline
      << "</select></label>" << endline << endline;
      
      // Set up options for grid lines on chart
      output << "<label><input type=\"checkbox\" onchange=\"ongridxchange(this,spec_chart_" << div_id << ")\" "
      << (options.m_showVerticalGridLines ?  "checked" : "") << ">Grid X</label>" << endline;
      output << "<label><input type=\"checkbox\" onchange=\"ongridychange(this,spec_chart_" << div_id << ")\" "
      << (options.m_showHorizontalGridLines ?  "checked" : "") << ">Grid Y</label>" << endline << endline;
      
      // Set up options for displaying peaks/title
      output << "<label><input type=\"checkbox\" onchange=\"showTitle(this,spec_chart_" << div_id << ")\" checked>Show Title</label>" <<endline << endline;
      
      // Set up option for displaying legend
      output << "<br />" << "<label>"
      << "<input id=\"legendoption\" type=\"checkbox\" onchange=\"setShowLegend(this,spec_chart_" << div_id << ");\" "
      << (options.m_legendEnabled ? "checked" : "") << ">Draw Legend</label>" << endline;
      
      // Set up option for compact x-axis
      output << "<label><input type=\"checkbox\" onchange=\"setCompactXAxis(this,spec_chart_" << div_id << ");\" "
      << (options.m_compactXAxis ? "checked" : "") << ">Compact x-axis</label>" << endline;
      
      // Set up option for mouse position statistics
      output << "<label><input type=\"checkbox\" onchange=\"setShowMouseStats(this,spec_chart_" << div_id << ");\" checked>Mouse Position stats</label>" << endline;
      
      // Set up option for animations
      //output << "<label><input type=\"checkbox\" onchange=\"setShowAnimation(this,spec_chart_" << div_id << ")\">Show zoom animation with duration: "
      //<< "<input type=\"number\" size=3 value=\"200\" min=\"0\" id=\"animation-duration\" "
      //<< "oninput=\"setAnimationDuration(this.value,spec_chart_" << div_id << ");\"><label>ms</label></label>" << endline << endline;
      //output << "<br /> ";
      
      output << endline << endline;
      
      output << "<br />" << endline
      << "<label><input type=\"checkbox\" onchange=\"setComptonEdge(this,spec_chart_" << div_id << ");\" " << (options.m_showComptonEdgeMarker ? "checked" : "")
      << ">Show compton edge</label>" << endline  // Set up option for compton edge marker
      << "<label><input type=\"checkbox\" onchange=\"setComptonPeaks(this,spec_chart_" << div_id << ");\" " << (options.m_showComptonPeakMarker ? "checked" : "")
      << ">Show compton peak energy with angle: <input type=\"number\" size=5 placeholder=\"180\" value=\"180\" max=\"180\" min=\"0\" id=\"angle-text\" oninput=\"setComptonPeakAngle(this.value,spec_chart_" << div_id << ");\"><label>degrees</label></label>" << endline // Set up option for compton peak energy marker
      << "<label><input type=\"checkbox\" onchange=\"setEscapePeaks(this,spec_chart_" << div_id << ");\" " << (options.m_showEscapePeakMarker ? "checked" : "")
      << ">Show escape peak energies</label>" << endline  // Set up option for escape peak marker
      << "<label><input type=\"checkbox\" onchange=\"setSumPeaks(this,spec_chart_" << div_id << ");\" " << (options.m_showSumPeakMarker ? "checked" : "") << ">Show sum peak energies</label>" << endline << endline;  // Set up option for sum peak marker
      
      //output << "<br />" << endline
      //<< "<label><input type=\"checkbox\" onchange=\"setBackgroundSubtract(this,spec_chart_" << div_id << ")\" " << (options.m_backgroundSubtract ? "checked" : "") << ">Background subtract</label>" << endline  // Set up option for background subtract
      //<< "<label><input id=\"scaleroption\" type=\"checkbox\" onchange=\"setSpectrumScaleFactorWidget(this,spec_chart_" << div_id << ")\">Enable scale background and secondary</label>" << endline // Set up option for spectrum y-scaler widget
      //<< "<label><input type=\"checkbox\" onchange=\"setShowXAxisSliderChart(this,spec_chart_" << div_id << ");\">Show x-axis slider chart</label>" << endline  // Set up option for x-axis slider chart widget
      //<< "<label><input type=\"checkbox\" onchange=\"setXRangeArrows(this,spec_chart_" << div_id << ")\" checked>Show x-axis range continuse arrows</label>" << endline << endline; // Set up option for x-axis range arrows
      
      // Set up option for spectrum scale factor widget
      //output << "<br />" << endline
      //<< "<label>Choose Spectrum to Scale: <select id=\"current-sf-title\" onchange=\"setDropDownSpectrumScaleFactor(this,spec_chart_" << div_id << ")\">" << endline
      //<< "<option value=\"None\">none</option></select></label>" << endline
      //<< "<label>Current Scale Factor: <input type=\"number\" min=\"0\" size=\"4\" id=\"current-sf\" oninput=\"setSpectrumScaleFactor(this,spec_chart_" << div_id << ");\"></label>" << endline
      //<< "<label>Max Scale Factor: <input id=\"max-sf\" type=\"number\" min=\"0.1\" id=\"bg-msf\" oninput=\"setMaxScaleFactor(this,spec_chart_" << div_id << ");\"></label>" << endline << endline
      //<< "</div>";
      
      output << "</div>" << endline;
    }//End manual write of options
    
    
    if( options.m_reference_lines_json.size() )
      output << "<script>onrefgammachange(document.getElementById('referenceGammaSelect"
      << div_id << "'),spec_chart_" << div_id << ",reference_lines_" << div_id << ");</script>" << endline;
    
    
    // Set up spectra options for scale factor widget
    output << "<script type=\"text/javascript\">"
    << "if (currentsftitle = document.getElementById('current-sf-title')) {"
    << "var titles = spec_chart_" << div_id << ".getSpectrumTitles();"
    << "currentsftitle.options.length = titles.length;"
    << "titles.forEach(function(title,i) {"
    << "if (i == 0) currentsftitle.options[i] = new Option('None', '', true, false);"
    << "else currentsftitle.options[i] = new Option(title, title, false, false);   });}</script>";
  }//for( const int sample : meas.sample_numbers() )
  
  
  output << "</body>" << endline;
  output << "</html>" << endline;
  
  return !output.bad();
}//writeIndividualHtmlSpectraToOutputFile(...)
#endif //#if( SpecUtils_ENABLE_D3_CHART )
  
  
bool writeSumOfSpectraToOutputFile( const SaveSpectrumAsType format,
                                   const QString initialDir,
                                   QString filename,
                                   std::shared_ptr<MeasurementInfo> meas,
                                   const std::set<int> samplenums,
                                   std::vector<bool> detectors,
                                   const QString nameAppend )
{
  bool ok = true;
  
  if( !meas )
    return false;
  
  if( nameAppend.size() )
  {
    QString extention;
    const int pos = filename.lastIndexOf( '.' );
    if( pos >= 0 )
    {
      extention = filename.right( filename.size() - pos );
      filename = filename.left( pos );
    }//if( pos >= 0 )
    
    filename = filename + "_" + nameAppend + extention;
  }//if( nameAppend.size() )
  
  
  QFileInfo outputfile( initialDir, filename );
  
  if( outputfile.exists() )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Existing File" );
    msg.setText( "File already exists, replace?\n"
                + outputfile.filePath() );
    msg.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
    const int code = msg.exec();
    if( code == QMessageBox::No )
      return false;
  }//if( outputfile.exists() )
  
  
  std::set<int> detnums;
  if( detectors.size() == meas->detector_numbers().size() )
  {
    for( size_t i = 0; i < detectors.size(); ++i )
      if( detectors[i] )
        detnums.insert( meas->detector_numbers()[i] );
  }else if( meas->detector_numbers().size() == 1 )
  {
    detnums.insert( meas->detector_numbers()[0] );
  }
  
  
  if( detnums.empty() )
  {
    qDebug() << "writeSumOfSpectraToOutputFile(...): tried to write"
    " file without specifying detectors to include.";
    return false;
  }//if( detnums.empty() )
  
  std::ofstream output( outputfile.filePath().toUtf8().data(),
                       std::ios::binary | std::ios::out );
  if( !output.is_open() )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "Unable to create output file: " + outputfile.filePath() );
    msg.exec();
    return false;
  }//if( !output.is_open() )
  
  switch( format )
  {
    case kTxtSpectrumFile: case kCsvSpectrumFile:
    case kPcfSpectrumFile: case kXmlSpectrumFile:
    case k2012N42SpectrumFile:
    case kExploraniumGr130v0SpectrumFile:
    case kExploraniumGr135v2SpectrumFile:
#if( SpecUtils_ENABLE_D3_CHART )
    case kD3HtmlSpectrumFile:
#endif
    {
      std::shared_ptr<Measurement> m
                              = meas->sum_measurements( samplenums, detectors );
      
      if( !m )
      {
        qDebug() << "Failed to sum meas";
        ok = false;
      }else
      {
        MeasurementInfo info = *meas;
        
        if( info.measurements().size() > 1 )
        {
          info.remove_measurments( info.measurements() );
          info.add_measurment( m, true );
        }//if( info.measurements().size() > 1 )
        
        switch( format )
        {
          case kTxtSpectrumFile:     ok = info.write_txt( output );        break;
          case kCsvSpectrumFile:     ok = info.write_csv( output );        break;
          case kPcfSpectrumFile:     ok = info.write_pcf( output );        break;
          case kXmlSpectrumFile:     ok = info.write_2006_N42( output );    break;
          case k2012N42SpectrumFile: ok = info.write_2012_N42( output ); break;
          case kExploraniumGr130v0SpectrumFile: ok = info.write_binary_exploranium_gr130v0( output ); break;
          case kExploraniumGr135v2SpectrumFile: ok = info.write_binary_exploranium_gr135v2( output ); break;
          
#if( SpecUtils_ENABLE_D3_CHART )
          case kD3HtmlSpectrumFile:
          {
            //If sample numbers are all sample numbers in the file, and there is one
            //  sample that is background, than that should be plotted seperately.
            //D3SpectrumExport::D3SpectrumChartOptions options;
            //Use default options
            //ok = meas->write_d3_html( output, options, set<int>(), set<int>() );
            
            vector<bool> detectors( info.detector_numbers().size(), true );
            ok = writeIndividualHtmlSpectraToOutputFile( output, info, info.sample_numbers(), detectors );
            break;
          }
#endif
            
          case kChnSpectrumFile:
          case kBinaryIntSpcSpectrumFile:
          case kBinaryFloatSpcSpectrumFile:
          case kNumSaveSpectrumAsType:
          case kIaeaSpeSpectrumFile:
          case kAsciiSpcSpectrumFile:
            break;
        }//switch( type )
      }//if( !m ) / else
      
      break;
    }//if( writing to a file that can handle multiple spectra )
      
    case kChnSpectrumFile:
    {
      ok = meas->write_integer_chn( output, samplenums, detnums );
      break;
    }//case kChnSpectrumFile:

    case kBinaryIntSpcSpectrumFile:
    {
      ok = meas->write_binary_spc( output, MeasurementInfo::IntegerSpcType, samplenums, detnums );
      break;
    }
      
    case kBinaryFloatSpcSpectrumFile:
    {
      ok = meas->write_binary_spc( output, MeasurementInfo::FloatSpcType, samplenums, detnums );
      break;
    }

    case kAsciiSpcSpectrumFile:
    {
      ok = meas->write_ascii_spc( output, samplenums, detnums );
      break;
    }
      
    case kIaeaSpeSpectrumFile:
    {
      ok = meas->write_iaea_spe( output, samplenums, detnums );
      break;
    }
      
    case kNumSaveSpectrumAsType:
      break;
  }//switch( format )
  
  return ok;
}//writeSumOfSpectraToOutputFile(...)

  


bool writeIndividualSpectraToOutputFile( const SaveSpectrumAsType format,
                                                    const QString initialDir,
                                                    const QString filename,
                                                    std::shared_ptr<MeasurementInfo> meas,
                                                    const std::set<int> samplenums,
                                                    std::vector<bool> detectors )
{
  bool ok = false;
  
  if( !meas )
    return false;
  
  QFileInfo outputfile( initialDir, filename );
  
  if( outputfile.exists() )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Existing File" );
    msg.setText( "File already exists, replace?\n"
                + outputfile.filePath() );
    msg.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
    const int code = msg.exec();
    if( code == QMessageBox::No )
      return false;
  }//if( outputfile.exists() )
  
  
  if( detectors.size() != meas->detector_numbers().size() )
  {
    qDebug() << "writeIndividualSpectraToOutputFile() called with invalid"
    " number of detector specifications: " << detectors.size()
    << " vs expected " << meas->detector_numbers().size();
    
    return false;
  }//if( detectors.size() != meas->detector_numbers().size() )
  
  
  //We'll write the file from a copy of the input MeasurementInfo, just in case
  //  we dont want to write anything (note, we could refactor to only make a
  //  copy of MeasurementInfo if necassarry, but to remain consistent, and since
  //  the copy should be much shorter time to make than the writing to disk, we
  //  will just always make the copy).
  MeasurementInfo info = *meas;
  
  bool alldets = (detectors.size()==meas->detector_numbers().size());
  for( const bool b : detectors )
    alldets = (alldets && b);
  
  if( !alldets || (samplenums.size() == meas->sample_numbers().size()) )
  {
    set<int> dets;
    for( size_t i = 0; i < detectors.size(); ++i )
      if( detectors[i] )
        dets.insert( meas->detector_numbers()[i] );
    
    MeasurementInfo info = *meas;
    vector<MeasurementConstShrdPtr> toremove;
    foreach( MeasurementConstShrdPtr oldm, info.measurements() )
    {
      if( !samplenums.count(oldm->sample_number())
         || !dets.count(oldm->detector_number()) )
      {
        toremove.push_back( oldm );
      }
    }//foreach( MeasurementConstShrdPtr oldm, info.measurements() )
    
    info.remove_measurments( toremove );
    
    
    if( info.measurements().empty() )
    {
      qDebug() << "writeIndividualSpectraToOutputFile() no spectrum left after"
      " filtering";
      return false;
    }
  }//if( we dont want everything in the input )
  
  const string utf8_outname = outputfile.filePath().toUtf8().data();
  
#ifdef _WIN32
  const std::wstring win_outname = UtilityFunctions::convert_from_utf8_to_utf16(utf8_outname);
  std::ofstream output( win_outname.c_str(), std::ios::binary | std::ios::out );
#else
  std::ofstream output( utf8_outname, std::ios::binary | std::ios::out );
#endif
  
  if( !output.is_open() )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "Unable to create output file: " + outputfile.filePath() );
    msg.exec();
    return false;
  }//if( !output.is_open() )
  
  
  switch( format )
  {
    case kTxtSpectrumFile:                ok = info.write_txt( output );        break;
    case kCsvSpectrumFile:                ok = info.write_csv( output );        break;
    case kPcfSpectrumFile:                ok = info.write_pcf( output );        break;
    case kXmlSpectrumFile:                ok = info.write_2006_N42( output );   break;
    case k2012N42SpectrumFile:            ok = info.write_2012_N42( output );   break;
    case kExploraniumGr130v0SpectrumFile: ok = info.write_binary_exploranium_gr130v0( output );   break;
    case kExploraniumGr135v2SpectrumFile: ok = info.write_binary_exploranium_gr135v2( output );   break;
#if( SpecUtils_ENABLE_D3_CHART )
    case kD3HtmlSpectrumFile:
    {
      if( samplenums.size() > 100 )
      {
        QMessageBox msg;
        msg.setWindowTitle( "Large Output File Warning" );
        
        msg.setText( QString("The output file will have %1 separate charts, which may tax some web browsers.\n"
                            "You could instead save with all records (or only the ones displayed in the spectrum tab) summed so the result will only have one chart.\n"
                            "Are you sure you would like to continue?").arg(samplenums.size()) );
        msg.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
        const int code = msg.exec();
        if( code == QMessageBox::No )
        {
          output.close();
          QFile file( outputfile.filePath() );
          file.remove();
          return false;
        }
      }//if( samplenums.size() > 20 )
      
      ok = writeIndividualHtmlSpectraToOutputFile( output, info, samplenums, detectors );
      break;
    }
#endif
      
    case kChnSpectrumFile:
    case kBinaryIntSpcSpectrumFile:
    case kBinaryFloatSpcSpectrumFile:
    case kAsciiSpcSpectrumFile:
    case kIaeaSpeSpectrumFile:
    {
      if( samplenums.size()==1 && detectors.size()==1 )
      {
        set<int> detnums;
        detnums.insert( info.detector_numbers()[0] );
        if( format == kChnSpectrumFile )
          ok = info.write_integer_chn( output, samplenums, detnums );
        else if( format == kBinaryIntSpcSpectrumFile )
          ok = info.write_binary_spc( output, MeasurementInfo::IntegerSpcType, samplenums, detnums );
        else if( format == kBinaryFloatSpcSpectrumFile )
          ok = info.write_binary_spc( output, MeasurementInfo::FloatSpcType, samplenums, detnums );
        else if( format == kAsciiSpcSpectrumFile )
          ok = info.write_ascii_spc( output, samplenums, detnums );
        else if( format == kIaeaSpeSpectrumFile )
          ok = info.write_iaea_spe( output, samplenums, detnums );
        else
          assert( 0 );
      }else
      {
        qDebug() << "writeIndividualSpectraToOutputFile() called for CHN/SPC "
                    "file  with samplenums.size() =" << samplenums.size()
                 << "and detectors.size() =" << detectors.size();
        ok = false;
      }
      
      break;
    }//case kChnSpectrumFile, kBinaryIntSpcSpectrumFile, kBinaryFloatSpcSpectrumFile:
      
    case kNumSaveSpectrumAsType:
      break;
  }//switch( type )
  
  
  output.close();
  
  return ok;
}//bool writeIndividualSpectraToOutputFile(...)

}//namespace


SaveWidget::SaveWidget( QWidget *parent , Qt::WindowFlags f )
  : QWidget( parent, f ),
    m_layout( 0 ),
    m_saveDirectory( 0 ),
    m_saveName( 0 ),
    m_saveAsType( 0 ),
    m_formatDescription( 0 ),
    m_noOptionsText( 0 ),
    m_saveToSingleSpecFileGroup( new QButtonGroup( this ) ),
    m_saveToSingleSpecFile( 0 ),
    m_saveToMultiSpecFileGroup( new QButtonGroup( this ) ),
    m_saveToMultiSpecFile( 0 ),
    m_saveButton( 0 )
{
  m_layout = new QGridLayout();
  setLayout( m_layout );
  
  QGroupBox *formatframe = new QGroupBox( "Save Format" );
  QGridLayout *formatlayout = new QGridLayout;
  formatframe->setLayout( formatlayout );
  m_saveAsType = new QListWidget();
  formatlayout->addWidget( m_saveAsType, 0, 0 );
  m_layout->addWidget( formatframe, 0, 0 );
  
  QGroupBox *optionsframe = new QGroupBox( "Options" );
  QGridLayout *optionslayout = new QGridLayout;
  optionsframe->setLayout( optionslayout );
  
  m_saveToSingleSpecFile = new QGroupBox("How");
  m_saveToMultiSpecFile = new QGroupBox("What samples to save:");
  optionslayout->addWidget( m_saveToSingleSpecFile, 0, 0 );
  optionslayout->addWidget( m_saveToMultiSpecFile, 1, 0 );
  
  m_noOptionsText = new QLabel( "No additional options" );
  optionslayout->addWidget( m_noOptionsText, 2, 0 );
  m_noOptionsText->hide();

  
  QGridLayout *optlayout = new QGridLayout();
  m_saveToSingleSpecFile->setLayout( optlayout );
  m_saveToSingleSpecFileGroup->setExclusive( true );
  
  for( WhatToSaveInSingleSpecFile type = WhatToSaveInSingleSpecFile(0);
      type < NumWhatToSaveInSingleSpecFile;
      type = WhatToSaveInSingleSpecFile(type+1) )
  {
    const char *txt = "";
    const char *tip = "";
    switch( type )
    {
      case kCurrentlyDisplayedOnly:
        txt = "Current record only";
        tip = "Only the currently displayed sample will be placed in the output"
              " file.";
      break;
        
      case kShowingSpectraSummedToSingle:
        txt = "Displayed spectra sum";
        tip = "The currently showing spectrum (which is a sum of multiple"
              " records) will be placed into the output file as a single"
              " record.";
      break;
        
      case kAllSpectraSummedToSingle:
        txt = "All spectra summed";
        tip = "All records in the input file will be summed to a single record"
              " in the output file.";
      break;
        
      case kEachSpectraInSeperateFile:
        txt = "To seperate files";
        tip = "Each record in the input file will be saved to a seperate output"
              " file.";
      break;
        
      case NumWhatToSaveInSingleSpecFile:
        break;
    }//switch( type )
    
    QRadioButton *radio = new QRadioButton( txt );
    radio->setToolTip( tip );
    m_saveToSingleSpecFileGroup->addButton( radio, type );
    optlayout->addWidget( radio, type, 0 );
    if( type == kEachSpectraInSeperateFile )
      radio->setChecked(true);
  }//for( loop over WhatToSaveInSingleSpecFile )
  
  
  optlayout = new QGridLayout();
  m_saveToMultiSpecFile->setLayout( optlayout );
  m_saveToMultiSpecFileGroup->setExclusive( true );
  
  for( WhatToSaveInMultiSpecFile type = WhatToSaveInMultiSpecFile(0);
       type < NumWhatToSaveInMultiSpecFile;
       type = WhatToSaveInMultiSpecFile(type+1) )
  {
    const char *txt = "";
    const char *tip = "";
    
    switch ( type )
    {
      case kAllSpecSeperatelyToSingleFile:
        txt = "All records to single file";
        tip = "All records in the input file will be saved to the output file"
              " with the minimum amount of changes possible.";
      break;
      
      case kCurrentSingleSpecOnly:
        txt = "Current showing record";
        tip = "Only the currently showing record will be saved to the output"
              " file.";
      break;
        
      case kShowingSpectraAsSingle:
        txt = "Displayed as single record";
        tip = "The currently showing spectrum (which is a sum of multiple"
              " records), will be saved as a single record in the output file.";
      break;
        
      case kAllSpecSummedToSingle:
        txt = "All records summed";
        tip = "All records in the input file will be summed to a single record"
              " placed into the output file.";
      break;
        
      case kEachSpecToSeperateFile:
        txt = "Each record to own file";
        tip = "Each record in the input file, will be saved to a seperate"
              " output file, with only one record in each file.";
      break;
        
      case kSumDetectorsForeachSampleNumber:
        txt = "Sum detectors per sample";
        tip = "For each sample time slice, the spectra will be summed from all"
              " of the detectors to result in a single record in the output"
              " file cooresponding to each sample slice in the input file.";
      break;
        
      case NumWhatToSaveInMultiSpecFile:
      break;
    }//switch ( type )
    
  
    QRadioButton *radio = new QRadioButton( txt );
    radio->setToolTip( tip );
    optlayout->addWidget( radio, type, 0 );
    m_saveToMultiSpecFileGroup->addButton( radio, type );
    
    if( type == kAllSpecSeperatelyToSingleFile )
      radio->setChecked(true);
  }//for( loop over WhatToSaveInMultiSpecFile )
  
  m_layout->addWidget( optionsframe, 0, 1 );
  
  m_saveToSingleSpecFile->hide();
  m_saveToMultiSpecFile->hide();

  QGroupBox *nameframe = new QGroupBox( "Name To Save As" );
  QGridLayout *namelayout = new QGridLayout;
  nameframe->setLayout( namelayout );
  
  QLabel *label = new QLabel( "Directory To Save To:" );
  namelayout->addWidget( label, 0, 0, Qt::AlignLeft );
  m_saveDirectory = new QLineEdit;
#if defined(__APPLE__) && !defined(IOS)
  m_saveDirectory->setEnabled( false );
  m_saveDirectory->setText( "Downloads" );
#endif
  namelayout->addWidget( m_saveDirectory, 1, 0 );
  QPushButton *browse = new QPushButton( "Browse..." );
  namelayout->addWidget( browse, 2, 0, Qt::AlignRight );
  
  label = new QLabel( "Name:" );
  namelayout->addWidget( label, 3, 0, Qt::AlignLeft );
  m_saveName = new QLineEdit();
  namelayout->addWidget( m_saveName, 4, 0 );
  
  m_saveButton = new QPushButton( "Save" );
  m_saveButton->setEnabled( false );
  namelayout->addWidget( m_saveButton, 5, 0, Qt::AlignCenter );
  
  namelayout->addWidget( new QWidget, 6, 0 );
  
  namelayout->setColumnStretch( 0, 10 );
  namelayout->setRowStretch( 6, 10 );
  
  m_layout->addWidget( nameframe, 0, 2 );
  
  QGroupBox *descframe = new QGroupBox( "Format Description" );
  QGridLayout *desclayout = new QGridLayout;
  descframe->setLayout( desclayout );
  
  QScrollArea *formatscroll = new QScrollArea();
  m_formatDescription = new QLabel();
  m_formatDescription->setAlignment( Qt::AlignTop | Qt::AlignLeft );
  formatscroll->setWidget( m_formatDescription );
  formatscroll->setWidgetResizable( true );
  formatscroll->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
  formatscroll->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
  m_formatDescription->setWordWrap( true );
  desclayout->addWidget( formatscroll, 0, 0 );
  m_layout->addWidget( descframe, 1, 0, 1, 3 );
  
  m_layout->setColumnStretch( 0, 1 );
  m_layout->setColumnStretch( 1, 1 );
  m_layout->setColumnStretch( 2, 1 );
  m_layout->setRowStretch( 0, 3 );
  m_layout->setRowStretch( 1, 2 );
  
  QFont serifFont("Helvetica", 16, QFont::Bold);
  
  QFontMetricsF metric( serifFont );
  
  qreal maxheight = 0.0, maxwidth = 0.0;
  for( SaveSpectrumAsType type = SaveSpectrumAsType(0);
      type < kNumSaveSpectrumAsType;
      type = SaveSpectrumAsType(type+1) )
  {
    QRectF bb = metric.boundingRect( description(type) );
    maxheight = std::max( maxheight, bb.height() );
    maxwidth = std::max( maxwidth, bb.width() );
  }
  
  QSize itemsize( static_cast<int>( ceil(maxwidth) ), 4 + static_cast<int>( ceil(maxheight) ) );
  
  for( SaveSpectrumAsType type = SaveSpectrumAsType(0);
       type < kNumSaveSpectrumAsType;
       type = SaveSpectrumAsType(type+1) )
  {
    QListWidgetItem *item = new QListWidgetItem( description(type) );
    item->setFont( serifFont );
    item->setTextAlignment( Qt::AlignVCenter );  //doesnt seem to make a difference
    item->setSizeHint( itemsize );
    m_saveAsType->addItem( item );
  }//for( loop over SaveSpectrumAsType )
  
  m_saveAsType->setSpacing( 2 );
  m_saveAsType->setUniformItemSizes( true );
  m_saveAsType->setSelectionRectVisible( true );

  
  QObject::connect( m_saveAsType, SIGNAL(itemSelectionChanged()), this, SLOT(formatChanged()) );
  QObject::connect( m_saveButton, SIGNAL(pressed()), this, SLOT(save()) );
  QObject::connect( browse, SIGNAL(pressed()), this, SLOT(browseForDirectory()) );
  
  QObject::connect( m_saveDirectory, SIGNAL(editingFinished()), this, SLOT(checkValidDirectory()) );
  
  m_saveAsType->setCurrentRow( k2012N42SpectrumFile );
}//SaveWidget constructior


SaveWidget::~SaveWidget()
{
}//~SaveWidget()



void SaveWidget::save()
{
  boost::scoped_ptr<BusyIndicator> indicator;
  CambioApp *app = dynamic_cast<CambioApp *>( qApp );
  
  if( app )
  {
    indicator.reset( new BusyIndicator( BusyIndicator::SavingSpectra,
                                        app->mainWindow() ) );
    indicator->show();
    repaint();
    indicator->repaint();
    update();
    indicator->update();
    app->processEvents();
  }//if( app )

  
  const int row = m_saveAsType->currentRow();
  if( !m_measurment )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "There is no spectrum currently loaded." );
    msg.exec();
    return;
  }
  
  if( row < 0 || row >= kNumSaveSpectrumAsType )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "Please select a save to type." );
    msg.exec();
    
    return;
  }//if( !!m_measurment || row < 0 || row >= kNumSaveSpectrumAsType )
  
  
  const QString initialDir = saveDirectory();
  
  QDir dir( initialDir );
  if( !dir.exists() || !dir.isReadable() )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "The selected save to directory is invalid or not readable,"
                 " please select another." );
    msg.exec();
    return;
  }//if( !dir.exists() || !dir.isReadable() )
  
  
  QString filename = m_saveName->text();
  if( filename.size() < 1 )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Error saving" );
    msg.setText( "Please enter a valid name to save to." );
    msg.exec();
    return;
  }//if( filename.size() < 1 )
  
  
  bool ok = false;
  const SaveSpectrumAsType type = SaveSpectrumAsType(row);
  
  
  if( type == kChnSpectrumFile
      || type == kBinaryIntSpcSpectrumFile
      || type == kBinaryFloatSpcSpectrumFile
      || type == kAsciiSpcSpectrumFile
      || type == kIaeaSpeSpectrumFile
     )
  {
    const int id = m_saveToSingleSpecFileGroup->checkedId();
    if( id < 0 )
    {
      QMessageBox msg;
      msg.setWindowTitle( "Error" );
      msg.setText( "No option of how to save to file is selected." );
      msg.exec();
      return;
    }//if( id < 0 )
    
    const WhatToSaveInSingleSpecFile method = WhatToSaveInSingleSpecFile(id);
    
    //In principle, the 'method' checked should be valid for this particular
    //  input situation, so we can relly on 'method'.
    switch( method )
    {
      case kCurrentlyDisplayedOnly:
      case kShowingSpectraSummedToSingle:
      {
        ok = writeSumOfSpectraToOutputFile( type, initialDir,
                                            filename, m_measurment,
                                            m_samplenums, m_detectors, "" );
        break;
      }//case kCurrentlyDisplayedOnly or kShowingSpectraSummedToSingle
        
      case kAllSpectraSummedToSingle:
      {
        const set<int> samples = m_measurment->sample_numbers();
        const vector<bool> dets( m_measurment->detector_names().size(), true );
        ok = writeSumOfSpectraToOutputFile( type, initialDir,
                                            filename, m_measurment,
                                            samples, dets, "" );
        break;
      }//case kAllSpectraSummedToSingle:
        
      case kEachSpectraInSeperateFile:
      {
        ok = true;
        int nwroteone = 1;
        
        foreach( MeasurementConstShrdPtr meas, m_measurment->measurements() )
        {
          if( !meas )
            continue;
          
          const QString nameappend = QString("%1").arg( nwroteone );
          set<int> samples;
          vector<bool> dets( m_measurment->detector_names().size(), false );
          
          samples.insert( meas->sample_number() );
          const vector<int> &detnums = m_measurment->detector_numbers();
          vector<int>::const_iterator pos;
          pos = std::find( detnums.begin(), detnums.end(), meas->detector_number() );
          const size_t detpos = pos - detnums.begin();
          
          if( pos != detnums.end() )
          {
            dets[detpos] = true;
            const bool wrote = writeSumOfSpectraToOutputFile(
                                                  type, initialDir,
                                                  filename, m_measurment,
                                                  samples, dets, nameappend );
            nwroteone += wrote;
            ok &= wrote;
          }else
          {
            //shouldnt ever get here
            ok = false;
            qDebug() << "Logic error finding detector number in SaveWidget::save()";
          }
        }//foreach( MeasurementConstShrdPtr meas, m_measurment->measurements() )
        
        break;
      }//case kEachSpectraInSeperateFile:
        
      case NumWhatToSaveInSingleSpecFile:
        break;
    }//switch( method )
    
  }else
  {
    const int id = m_saveToMultiSpecFileGroup->checkedId();
    if( id < 0 )
    {
      QMessageBox msg;
      msg.setWindowTitle( "Error" );
      msg.setText( "No option of how to save to file is selected." );
      msg.exec();
      return;
    }//if( id < 0 )
    
    const WhatToSaveInMultiSpecFile method = WhatToSaveInMultiSpecFile(id);
    
    switch( method )
    {
      case kAllSpecSeperatelyToSingleFile:
      {
        const size_t ndets = m_measurment->detector_numbers().size();
        const vector<bool> alldets( ndets, true );
        const set<int> allsamples = m_measurment->sample_numbers();
        
        ok = writeIndividualSpectraToOutputFile( type, initialDir, filename,
                                            m_measurment, allsamples, alldets );
        break;
      }//case kAllSpecSeperatelyToSingleFile:
        
      case kCurrentSingleSpecOnly:
      {
        ok = writeIndividualSpectraToOutputFile( type, initialDir, filename,
                                      m_measurment, m_samplenums, m_detectors );
        break;
      }//case kCurrentSingleSpecOnly:
        
      case kShowingSpectraAsSingle:
      {
        if( (m_measurment->sample_numbers().size()==1)
            && (m_measurment->detector_numbers().size()==1) )
        {
          ok = writeIndividualSpectraToOutputFile( type, initialDir, filename,
                                                m_measurment,
                                                m_measurment->sample_numbers(),
                                                vector<bool>(1,true) );
        }else
        {
          ok = writeSumOfSpectraToOutputFile( type, initialDir, filename,
                                  m_measurment, m_samplenums, m_detectors, "" );
        }
        break;
      }//case kShowingSpectraAsSingle:
        
      case kAllSpecSummedToSingle:
      {
        const size_t ndets = m_measurment->detector_numbers().size();
        const vector<bool> alldets( ndets, true );
        const set<int> allsamples = m_measurment->sample_numbers();
        ok = writeSumOfSpectraToOutputFile( type, initialDir, filename,
                                       m_measurment, allsamples, alldets, "" );
        break;
      }//case kAllSpecSummedToSingle:
        
      case kEachSpecToSeperateFile:
      {
        ok = true;
        int nwroteone = 1;
        
        if( m_measurment->measurements().size() > 20 )
        {
          QMessageBox msg;
          msg.setWindowTitle( "Many Output Files Warning" );
          msg.setText( QString("This operation will create %1 separate files.\n"
                               "Are you sure you would like to continue?").arg(m_measurment->measurements().size()) );
          msg.setStandardButtons( QMessageBox::Yes | QMessageBox::No );
          const int code = msg.exec();
          if( code == QMessageBox::No )
            return;
        }//if( samplenums.size() > 20 )
        
        foreach( MeasurementConstShrdPtr meas, m_measurment->measurements() )
        {
          if( !meas )
            continue;
          
          const QString nameappend = QString("%1").arg( nwroteone );
          set<int> samples;
          vector<bool> dets( m_measurment->detector_names().size(), false );
          
          samples.insert( meas->sample_number() );
          const vector<int> &detnums = m_measurment->detector_numbers();
          vector<int>::const_iterator pos;
          pos = std::find( detnums.begin(), detnums.end(), meas->detector_number() );
          const size_t detpos = pos - detnums.begin();
          
          if( pos != detnums.end() )
          {
            dets[detpos] = true;
            const bool wrote = writeSumOfSpectraToOutputFile(
                                                             type, initialDir,
                                                             filename, m_measurment,
                                                             samples, dets, nameappend );
            nwroteone += wrote;
            ok &= wrote;
          }else
          {
            //shouldnt ever get here
            ok = false;
            qDebug() << "Logic error finding detector number in SaveWidget::save()";
          }
        }//foreach( MeasurementConstShrdPtr meas, m_measurment->measurements() )

        break;
      }//case kEachSpecToSeperateFile:
        
      case kSumDetectorsForeachSampleNumber:
      {
        std::shared_ptr<MeasurementInfo> info( new MeasurementInfo(*m_measurment) );
        
        const vector<bool> all_dets( m_measurment->detector_numbers().size(), true );
        const set<int> sample_nums = m_measurment->sample_numbers();
        
        vector<MeasurementShrdPtr> samplemeas;
        
        foreach( const int sample, sample_nums )
        {
          set<int> num;
          num.insert( sample );
          MeasurementShrdPtr m = m_measurment->sum_measurements( num, all_dets );
          if( m )
            samplemeas.push_back( m );
        }//foreach( const int sample, sample_nums )
        
        info->remove_measurments( info->measurements() );
        
        foreach( MeasurementShrdPtr m, samplemeas )
          info->add_measurment( m, false );
        info->cleanup_after_load(); //necassarry
        
        if( info->measurements().size() )
        {
          const vector<bool> dets( info->detector_numbers().size(), true );
          ok = writeIndividualSpectraToOutputFile( type, initialDir, filename,
                                                   info, info->sample_numbers(),
                                                   dets );
        }//if( info->measurements().size() )
        
        break;
      }//case kSumDetectorsForeachSampleNumber:
        
      case NumWhatToSaveInMultiSpecFile:
        break;
    }//switch( method )
  
  }//if( save to single spectrum file ) / else
  
  
  if( !ok )
  {
    QMessageBox msg;
    msg.setWindowTitle( "Possible error saving" );
    msg.setText( "Possible error saving " + filename
                + ", please check results" );
    msg.exec();
    return;
  }else
  {
    QMessageBox msg;
    msg.setWindowTitle( "Succes" );
    msg.setText( "'" + filename + "' Saved." );
    msg.exec();
    return;
  }//if( !ok )
}//void save()


void SaveWidget::checkValidDirectory()
{
  const QString initialDir = saveDirectory();
  
  QDir dir( initialDir );
  if( dir.exists() && dir.isReadable() )
    return;
  
#if defined(__APPLE__) && !defined(IOS)
  const QString downloadsFolder = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
  m_saveDirectory->setText(  "Downloads" );
#else
  m_saveDirectory->setText( QDir::homePath() );
#endif
  
  QMessageBox errormessage(this);
  errormessage.setWindowTitle( "Error" );
  errormessage.setText( initialDir + " is an invalid path, "
                       "the path has been changed to your home directory" );
  errormessage.exec();
}//void checkValidDirectory()


QString SaveWidget::saveDirectory()
{
  QString p = m_saveDirectory->text();
  
#if defined(__APPLE__) && !defined(IOS)
  if( p == "Downloads" )
    return QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
#endif
  
  QDir savedir( p );
  if( !savedir.exists() || !savedir.isReadable() )
    return "";

  return p;
}//QString saveDirectory()


void SaveWidget::browseForDirectory()
{
  const QString caption = "Select directory to save results to";
  const QString initialDir = saveDirectory();
  QFileDialog dialog( this, caption, initialDir );
  dialog.setFileMode( QFileDialog::DirectoryOnly );
  dialog.setViewMode( QFileDialog::Detail );
  
#if defined(ANDROID) || defined(IOS)
  const QRect geom = QApplication::desktop()->availableGeometry();
  dialog.resize( geom.width(), geom.height() );
#endif

  const int status = dialog.exec();
  
  if( status != QDialog::Accepted )
    return;

//  QString dirname = QFileDialog::getExistingDirectory( this, caption, initialDir, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks );
  
  QStringList selecteddirs = dialog.selectedFiles();
  if( selecteddirs.length() > 0 )
  {
    QSettings settings;
#if defined(__APPLE__) && !defined(IOS)
    if( QDir(selecteddirs[0]).exists() )
      settings.setValue( "SaveFileDir", selecteddirs[0] );
#endif
    
    
#if defined(__APPLE__) && !defined(IOS)
    const QString downloadsFolder = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
    if( QDir(selecteddirs[0]) == QDir(downloadsFolder) )
      m_saveDirectory->setText( "Downloads" );
    else
      m_saveDirectory->setText( selecteddirs[0] );
#else
    m_saveDirectory->setText( selecteddirs[0] );
#endif
  }//if( selecteddirs.length() > 0 )
}//void browseForDirectory()e


void SaveWidget::formatChanged()
{
  const int row = m_saveAsType->currentRow();
  if( row < 0 || row >= kNumSaveSpectrumAsType )
    return;
  
  SaveSpectrumAsType type = SaveSpectrumAsType(row);
  const char *desc = "";
  QString currname = m_saveName->text();
  const int pos = currname.lastIndexOf( '.' );
  if( pos > 0 )
    currname = currname.left( pos );
    
  
  switch( type )
  {
    case kTxtSpectrumFile:
      if( currname.size() )
        currname += ".txt";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "Spectrum(s) will be written to an ascii text file.  At the"
             " beggining of the file the original file name, total live and"
             " real times, sum gamma counts, sum neutron counts, and any file"
             " level remarks will be written on seperate labeled lines."
             "  Then after two blank lines each spectrum in the current file"
             " will be written, seperated by two blank lines."
             "  Each spectrum will contain all remarks, measurment start time"
             " (if valid), live and real times, sample number, detector name,"
             " detector type, GPS coordinates/time (if valid), serial number"
             " (if present), energy"
             " calibration type and coefficient values, and neutron counts (if"
             " valid); the"
             " channel number, channel lower energy, and channel counts is then"
             " provided with each channel being placed on a seperate line and"
             " each field being seperated by a space.\n\r"
             "Any detector provided analysis in the original program, as well"
             " manufacturer, UUID, deviation pairs, lane information,"
             " location name, or spectrum title is lost.\n\r"
             "Cambio or other programs may not be able to read back in all"
             " information written to the txt file.\n\r"
             "The Windows line ending convention is used (\\n\\r).\n"
             "This is not a standard format commonly read by other programs, "
             " and is intended as a easily human readable summary of the"
             " spectrum file information";
    break;
      
    case kCsvSpectrumFile:
      if( currname.size() )
        currname += ".csv";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "The spectra in the current file are written out in a two column"
             " format (seperated by a comma); the first column is gamma channel"
             " lower edge energy, the second column is channel counts.  Each"
             " spectrum in the file are written out contiguously and seperated"
             " by a header that reads \"Energy, Data\".  Windows style line"
             " endings are used (\\n\\r).\n"
             "This format loses all non-spectral information, including live"
             " and real times, and is intended to be an easy way to import the"
             " spectral information into other programs like Excel.\n";
    break;
      
    case kPcfSpectrumFile:
      if( currname.size() )
        currname += ".pcf";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "The PCF format is the binary native format of GADRAS.  Saving to"
             " this format will cause the loss of some information."
             "  However, Calibration, foreground/background, speed, sample, and"
             " spectrum title (up to 60 characters) will be preserved along"
             " with the spectral information and neutron counts.";
    break;
      
    case kXmlSpectrumFile:
      if( currname.size() )
        currname += ".n42";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "Saves to the 2006 N42 XML format.  Nearly all relevant"
             " information in most input spectrum files will also be saved in"
             " the output file, however not all spectroscopy programs are able"
             " to extract all fields, or read all variations of N42 files.\n"
             "The produced file will be similar to simple spectrometer style"
             " file, but with some added tags to support the additional"
             " information.";
    break;
      
    case k2012N42SpectrumFile:
      if( currname.size() )
        currname += ".n42";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "Saves to the 2011 N42 XML format.  Nearly all relevant"
             " information in most input spectrum files will also be saved in"
             " the output file.";
      break;
      
    case kChnSpectrumFile:
      if( currname.size() )
        currname += ".chn";
      m_saveToMultiSpecFile->hide();
      m_saveToSingleSpecFile->show();
      desc = "This options produces an integer binary CHN file.  This format"
             " holds a single spectrum, so you are given the option to save"
             " either only the currently displayed spectrum, or save a seperate"
             " output file for each spectrum in the current input file.\n"
             "This format preserves the gamma spectrum,"
             " measurment start time, spectrum title (up to 63 characters),"
             " detector description, and energy calibration. Energy deviation"
             " pairs and neutron counts, as well as any other meta information"
             " is not preserved.";
      break;
      
    case kBinaryIntSpcSpectrumFile:
      if( currname.size() )
        currname += ".spc";
      m_saveToMultiSpecFile->hide();
      m_saveToSingleSpecFile->show();
      desc = "This options produces an integer binary SPC file.  This format"
             " holds a single spectrum, so you are given the option to save"
             " either only the currently displayed spectrum, or save a seperate"
             " output file for each spectrum in the current input file.\n"
             "This format preserves the gamma spectrum, neutron counts,"
             " gps info, measurment start time, detector serial number,"
             " and energy calibration.  Energy deviation pairs, analysis  "
             " results, and other meta information is not preserved.";
    break;

    case kBinaryFloatSpcSpectrumFile:
      if( currname.size() )
        currname += ".spc";
      m_saveToMultiSpecFile->hide();
      m_saveToSingleSpecFile->show();
      desc = "This options produces an floating point binary SPC file.  This format"
             " holds a single spectrum, so you are given the option to save"
             " either only the currently displayed spectrum, or save a seperate"
             " output file for each spectrum in the current input file.\n"
             "This format preserves the gamma spectrum, neutron counts,"
             " gps info, measurment start time, detector serial number,"
             " and energy calibration.  Energy deviation pairs, analysis  "
             " results, and other meta information is not preserved.";
      break;
    
    case kAsciiSpcSpectrumFile:
      if( currname.size() )
        currname += ".spc";
      m_saveToMultiSpecFile->hide();
      m_saveToSingleSpecFile->show();
      desc = "This options produces an ASCII SPC file.  This format"
      " holds a single spectrum, so you are given the option to save"
      " either only the currently displayed spectrum, or save a seperate"
      " output file for each spectrum in the current input file.\n"
      "This format preserves the gamma spectrum, neutron counts,"
      " gps info, measurment start time, detector serial number,"
      " and energy calibration.  Energy deviation pairs, some analysis"
      " information results, and possibly some, but not all, meta information"
      " will be lost.";
      break;
      
    case kExploraniumGr130v0SpectrumFile:
      if( currname.size() )
        currname += ".dat";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "The GR130 format is the binary native format for Exploranium GR130"
             " detectors.  Saving to this format will cause the data to be"
             " rebined to 256 channels, preserving the start and live times, but"
             " energy calibration and neutron information will be lost, along"
             " with all meta information such as comments, gps, etc. "
             " Multiple spectra can be written to a single file, with the"
             " records ordered by sample number then detector number.";
      break;
      
    case kExploraniumGr135v2SpectrumFile:
      if( currname.size() )
        currname += ".dat";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "The GR135 format is the binary native format for Exploranium GR135v2"
      " detectors.  Saving to this format will cause the data to be"
      " rebined to 1024 channels, preserving the start and live times, neutron"
      " counts, energy calibration, and numeric serial numbers."
      " Deviation pairs, comments, title, gps, and other meta information will"
      " be lost. Multiple spectra can be written to a single file, with the"
      " records ordered by sample number then detector number.";
      break;
      
    case kIaeaSpeSpectrumFile:
      if( currname.size() )
        currname += ".spe";
      m_saveToMultiSpecFile->hide();
      m_saveToSingleSpecFile->show();
      desc = "SPE files are ASCII based, single spectrum files common for"
             " exchanging data between programs. You are given the option to"
             " save either only the currently displayed spectrum, or save a"
             " seperate output file for each spectrum in the current input file.\n"
             "Saving to this format preserves neutron counts, start time, live"
             " time, real time, energy calibration, comments, and title."
             " Deviation pairs, gps, model, and other meta information will"
             " be lost";
      break;
      
#if( SpecUtils_ENABLE_D3_CHART )
    case kD3HtmlSpectrumFile:
      if( currname.size() )
        currname += ".html";
      m_saveToMultiSpecFile->show();
      m_saveToSingleSpecFile->hide();
      desc = "An stand-alone HTML file is produced which contains an interative"
             " spectrum chart and some basic viewing options.  If the spectrum"
             " file contains a background record it may be plotted as a"
             " seperate line on the same chart as the foreground spectra."
             " This file format can only be read and displayed by web browsers"
             " that have javascript enabled and support SVG images.";
      break;
#endif
    
    case kNumSaveSpectrumAsType:
      break;
  }//switch( type )

  m_saveName->setText( currname );
  m_formatDescription->setText( desc );
  
  if( !m_measurment || (m_measurment->measurements().size() < 2) )
  {
    m_saveToMultiSpecFile->hide();
    m_saveToSingleSpecFile->hide();
  }else
  {
    int ndetshow = 0;
    foreach( const bool d, m_detectors )
      ndetshow += d;
    
    for( WhatToSaveInSingleSpecFile type = WhatToSaveInSingleSpecFile(0);
        type < NumWhatToSaveInSingleSpecFile;
        type = WhatToSaveInSingleSpecFile(type+1) )
    {
      QAbstractButton *button = m_saveToSingleSpecFileGroup->button( type );
      
      switch( type )
      {
        case kCurrentlyDisplayedOnly:
          button->setHidden( !(ndetshow<2 && m_samplenums.size()<2) );
          button->setChecked( ndetshow<2 && m_samplenums.size()<2 );
        break;
          
        case kShowingSpectraSummedToSingle:
          button->setHidden( ndetshow<2 && m_samplenums.size()<2 );
        break;
          
        case kAllSpectraSummedToSingle:
          button->setHidden( false );
          button->setChecked( !(ndetshow<2 && m_samplenums.size()<2) );
        break;
          
        case kEachSpectraInSeperateFile:
          button->setHidden( false );
        break;
          
        case NumWhatToSaveInSingleSpecFile:
        break;
      }//switch( type )
    }//for( loop over WhatToSaveInSingleSpecFile )

    
    for( WhatToSaveInMultiSpecFile type = WhatToSaveInMultiSpecFile(0);
        type < NumWhatToSaveInMultiSpecFile;
        type = WhatToSaveInMultiSpecFile(type+1) )
    {
      QAbstractButton *button = m_saveToMultiSpecFileGroup->button( type );
      
      switch( type )
      {
        case kAllSpecSeperatelyToSingleFile:
          button->setChecked( true );
        break;
          
        case kCurrentSingleSpecOnly:
          button->setHidden( m_samplenums.size()>1 || ndetshow>1 );
        break;
          
        case kShowingSpectraAsSingle:
          button->setHidden( m_samplenums.size()<2 && ndetshow<2 );
        break;
          
        case kAllSpecSummedToSingle:
          button->setHidden( false );
        break;
          
        case kEachSpecToSeperateFile:
          button->setHidden( false );
        break;
    
        case kSumDetectorsForeachSampleNumber:
          button->setHidden( m_detectors.size()>1 );
        break;
          
        case NumWhatToSaveInMultiSpecFile:
        break;
      }//switch( type )
    }//for( loop over WhatToSaveInMultiSpecFile );
  }//if( a simple single spectrum file ) / else
  
  m_noOptionsText->setHidden( !m_saveToSingleSpecFile->isHidden()
                              || !m_saveToMultiSpecFile->isHidden() );
  
}//void formatChanged()


void SaveWidget::updateDisplay( std::shared_ptr<MeasurementInfo> meas,
                                std::set<int> samplenums,
                                std::vector<bool> detectors )
{
  m_samplenums.swap( samplenums );
  m_detectors.swap( detectors );
  m_measurment = meas;
  
  QString path = saveDirectory();
  
  m_saveName->setText( meas->filename().c_str() );

#if defined(__APPLE__) && !defined(IOS)
  if( path.isEmpty() )
    m_saveDirectory->setText( "Downloads" );
#else
  QSettings settings;
  if( path.size() < 2 )
  {
    QVariant var = settings.value( "SaveFileDir" );
     const QString val = var.isValid() ? var.toString() : QString("");
     if( val.size()>2 && QDir(val).exists() )
       path = val;

     QFileInfo file( path, meas->filename().c_str() );

     if( path.size() < 2 && file.exists() )
     {
       path = file.absolutePath();
#if( defined(ANDROID) )
       if( path.startsWith("/data") )
         path = "";
#endif
     }//if( path.size() < 2 && file.exists() )

     if( path.size() < 2 )
       path = QStandardPaths::writableLocation( QStandardPaths::DownloadLocation );
  }//if( m_saveDirectory->text().size() < 1 )
#endif

  QDir dir( path );
  m_saveButton->setEnabled( !!m_measurment && dir.exists() && dir.isReadable() );

  if( !m_measurment )
    return;

  
#if defined(__APPLE__) && !defined(IOS)
  //Apple sandbox means we wont be able to access the same a directory next time
  //  we open - so dont bother saving it
#else
  if( path != saveDirectory() )
  {
    settings.setValue( "SaveFileDir", path );
    m_saveDirectory->setText( path );
  }
#endif
  
  formatChanged();
}//void updateDisplay(...)


void SaveWidget::initBatchConvertion( QString directory )
{
  QList<QString> files;
  files.push_back( directory );
  
  initBatchConvertion( files );
}//void initBatchConvertion( QString directory )


void SaveWidget::initBatchConvertion( QList<QString> files )
{
  if( files.size() == 0 )
    return;
  
  BatchConvertDialog *batch = new BatchConvertDialog( this );
  batch->setAttribute( Qt::WA_DeleteOnClose, true );

  const bool isdir = (files.size()==1 && QFileInfo(files[0]).isDir());

  if( isdir )
    batch->setDirectory( files[0] );
  else
    batch->setFiles( files );
  
//  batch->exec();
  batch->show();
  batch->raise();
  batch->activateWindow();
}//void initBatchConvertion( QList<QString> files )




