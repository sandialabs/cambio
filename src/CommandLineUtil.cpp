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
#include <deque>
#include <cctype>
#include <string>
#include <deque>
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "SpecUtils_config.h"
#include "SpecUtils/SpecFile.h"
#include "SpecUtils/StringAlgo.h"
#include "SpecUtils/Filesystem.h"
#if( SpecUtils_ENABLE_URI_SPECTRA )
#include "SpecUtils/UriSpectrum.h"
#endif
#include "SpecUtils/EnergyCalibration.h"

#include "cambio/CommandLineUtil.h"


// Some includes to get terminal width
#if defined(__APPLE__) || defined(linux) || defined(unix) || defined(__unix) || defined(__unix__)
#include <sys/ioctl.h>
#include <unistd.h>
#elif defined(_WIN32)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif


using namespace std;
namespace po = boost::program_options;
using SpecUtils::convert_from_utf8_to_utf16;


#if( SpecUtils_ENABLE_D3_CHART )
#include "SpecUtils/D3SpectrumExport.h"

/** If you want to also save meta-info to JSON output (serial number, like GPS,
    analysis results, etc), then set WRITE_JSON_META_INFO to 1.
    Not tested, and only file-level (not record-leve) info currently written out.
 */
#define WRITE_JSON_META_INFO 0

namespace {
#if( !SpecUtils_D3_SUPPORT_FILE_STATIC )
  string file_to_string( const char *filename )
  {
#ifdef _WIN32
    ofstream file( convert_from_utf8_to_utf16(filename).c_str(), ios::in | ios::binary );
#else
    ofstream file( filename.c_str(), ios::in | ios::binary );
#endif
    
    if( !t.is_open() )
      throw runtime_error( "file_to_string: Failed to open '" + string(filename) + "'" );
    
    return std::string((std::istreambuf_iterator<char>(t)),
                       std::istreambuf_iterator<char>());
  }
#endif
  

  bool html_page_header( std::ostream &ostr, const std::string &title )
  {
    const char *endline = "\r\n";
    
    ostr << "<!DOCTYPE html><html>" << endline;
    ostr << "<head>" << endline;
    ostr << "<title>" << title << "</title>" << endline;
    
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
    ostr << "<script>" << D3SpectrumExport::d3_js() << "</script>" << endline;
    ostr << "<script>" << D3SpectrumExport::spectrum_chart_d3_js() << "</script>" << endline;
    ostr << "<script>" << D3SpectrumExport::spectrum_chart_setup_js() << "</script>" << endline;
    ostr << "<style>" << D3SpectrumExport::spectrum_char_d3_css() << "</style>" << endline;
#else
    ostr << "<script>" << file_to_string( D3SpectrumExport::d3_js_filename() ) << "</script>" << endline;
    ostr << "<script>" << file_to_string( D3SpectrumExport::spectrum_chart_d3_js_filename() ) << "</script>" << endline;
    ostr << "<script>" << D3SpectrumExport::spectrum_chart_setup_js() << "</script>" << endline;
    ostr << "<style>" << file_to_string( D3SpectrumExport::spectrum_char_d3_css_filename() ) << "</style>" << endline;
#endif
    
    ostr << "</head>" << endline;
    
    return ostr.good();
  }//html_page_header
  
#if( WRITE_JSON_META_INFO )
  /** Output meta-information at the file level */
  void add_file_meta_info_to_json( std::ostream &output, const SpecUtils::SpecFile &info )
  {
    auto jsonEscape = []( string input ) -> string {
      SpecUtils::ireplace_all( input, "\"", "\\\"" );  //ToDo: Properly escape JSON strings!
      return input;
    };
    
    output << "{ IsMetaInformation: true";
    if( !info.manufacturer().empty() )
      output << ", Manufacturer: \"" << jsonEscape( info.manufacturer() ) << "\"";
    if( !info.instrument_model().empty() )
      output << ", Model: \"" << jsonEscape( info.instrument_model() ) << "\"";
    output << ", DetectorType: \"" << jsonEscape( detectorTypeToString( info.detector_type() ) ) << "\"";
    if( !info.instrument_type().empty() )
      output << ", InstrumentType: \"" << jsonEscape( info.instrument_type() ) << "\"";
    if( !info.instrument_id().empty() )
      output << ", SerialNumber: \"" << jsonEscape( info.instrument_id() ) << "\"";
    output << ", FileUuid: \"" << jsonEscape( info.uuid() ) << "\"";
    
    const vector<string> &fileRemarks = info.remarks();
    if( fileRemarks.size() )
    {
      output << ", Remarks: [";
      for( size_t remarknum = 0; remarknum < fileRemarks.size(); ++remarknum )
        output << (remarknum ? "," : "") << "\"" << jsonEscape( fileRemarks[remarknum] ) << "\"";
      output << "]";
    }
    
    if( !info.measurement_operator().empty() )
      output << ", Operator: \"" << jsonEscape( info.measurement_operator() ) << "\"";
    
    output << ", NumberMeasurements: " << info.num_measurements();
    
    if( info.has_gps_info() )
    {
      //info.position_time()
      output << ", MeanLatitude: " << info.mean_latitude();
      output << ", MeanLongitude: " << info.mean_longitude();
    }
    //info.inspection()
    //info.measurement_location_name()
    //info.lane_number()
    //const std::vector<std::string> &SpecUtils::SpecFile::detector_names() const
    output << ", TotalLiveTime: " << info.gamma_live_time();
    output << ", TotalRealTime: " << info.gamma_real_time();
    output << ", TotalSumGammas: " << info.gamma_count_sum();
    //output << ", TotalSumNeutrons: " << info.neutron_counts_sum();
    
    std::shared_ptr<const DetectorAnalysis> ana = info.detectors_analysis();
    if( ana )
    {
      output << ", Analysis: {";
      
      output << " AlgorithmName:'" << jsonEscape( ana->algorithm_name_ ) << "'";
      output << " AlgorithmCreator:'" << jsonEscape( ana->algorithm_creator_ ) << "'";
      output << " AlgorithmDescription:'" << jsonEscape( ana->algorithm_description_ ) << "'";
      output << " AlgorithmResultDescription:'" << jsonEscape( ana->algorithm_result_description_ ) << "'";
      if( !ana->algorithm_component_versions_.empty() )
      {
        output << ", ComponentVersions: [";
        for( size_t i = 0; i < ana->algorithm_component_versions_.size(); ++i )
          output << (i?",":"") << "{component:'" << jsonEscape( ana->algorithm_component_versions_[i].first ) << "', "
                 << "version:'" << jsonEscape( ana->algorithm_component_versions_[i].second ) << "'}";
        output << "]";
      }
      
      if( !ana->remarks_.empty() )
      {
        output << ", Remarks: [";
        for( size_t i = 0; i < ana->remarks_.size(); ++i )
          output << (i?",":"") << "'" << jsonEscape( ana->remarks_[i] ) << "'";
        output << "]";
      }
      
      if( !ana->results_.empty() )
      {
        output << ", Results: [";
        for( size_t i = 0; i < ana->results_.size(); ++i )
        {
          const auto &result = ana->results_[i];
          output << (i?",":"") << "{";
          output << "Nuclide:'" << jsonEscape( result.nuclide_ ) << "'";
          if( !result.remark_.empty() )
            output << ", Remark:'" << jsonEscape( result.remark_ ) << "'";
          if( result.activity_ > 0.0f )
            output << ", Activity:" << result.activity_ << ""; //in units of becquerel (eg: 1.0 == 1 bq)
          //float ;
          if( !result.nuclide_type_.empty() )
            output << ", NuclideType:'" << jsonEscape( result.nuclide_type_ ) << "'";
          if( !result.id_confidence_.empty() )
            output << ", Confidence:'" << jsonEscape( result.id_confidence_ ) << "'";
          //float distance_;            //in units of mm (eg: 1.0 == 1 mm )
          if( result.dose_rate_ > 0.0f )
            output << ", DoseRate:" << result.dose_rate_ << ""; //in units of micro-sievert per hour
          //float real_time_;           //in units of seconds (eg: 1.0 = 1 s)
          //std::string detector_;
          output << "}";
        }
        output << "]";
      }//if( !ana->results_.empty() )
    }//if( ana )

    output << "},\n\t";
  }//void add_file_meta_info_to_json( std::ostream &ostr, const SpecUtils::SpecFile &info )
#endif
}
#endif

namespace
{
  
  /** Checks if the starting 3 or 4 letters are consistent with N42 naming scheme
     Doesnt check letters after this incase we have already prepended the name
     with the N42 check, or energy cal info got appended to name.
     Note: This function is case independant (e.g, "ba3" is acceptable).
   */
  bool is_n42_name( const std::string &name )
  {
    if( name.size() < 3 )
      return false;
  
    const bool first_okay = ((name[0] >= 'A' && name[0] <= 'D') || (name[0] >= 'a' && name[0] <= 'd'));
    const bool second_okay = ((name[1] >= 'A' && name[1] <= 'H') || (name[1] >= 'a' && name[1] <= 'h'));
    const bool third_okay = (name[2] >= '1' && name[2] <= '8');
    const bool fourth_okay = ((name.size() < 4) || name[3] == 'n' || name[3] == 'N' || name[3] == ' ' || name[3] == '_' );
    
    return first_okay && second_okay && third_okay && fourth_okay;
  }//bool is_n42_name( const std::string &name )
  
  /*
   //std::set<string,ilessthan()> doesnt seem to work
  struct ilessthan
  {
    ilessthan(){};
    
    bool operator()( const string &lhs, const string &rhs ) const {
      string lhs_copy = lhs, rhs_copy = rhs;
      std::transform( begin(lhs), end(lhs), begin(lhs),
                     [](unsigned char c){ return std::tolower(c); });
      std::transform( begin(rhs), end(rhs), begin(rhs),
                     [](unsigned char c){ return std::tolower(c); });
      return lhs < rhs;
    };
  };
   */
  
  /** If necessary, renames detectors so the start of detector names are
   consistent with N42 naming convention - primarily useful for applications
   that rely on detector names.
   
   @param info The measurement to try normalizing the names of
   @param dont_change_det_names Detectors names to not change, regardless of if
          they are N42 compliant.  Note: case sensitive.
   
   Note: If changes are needed, the detector name is prepended with a N42 name,
         followed by a space, then the original detector name.
   
   Note: This function is really a hack at the moment; there is room for much
         improvement.
   */
  void normalize_det_name_to_n42( SpecUtils::SpecFile &info, const vector<string> &dont_change_det_names )
  {
    const bool print_debug = false;
    
    //We will only change spectroscopic gamma detector names, and their
    //  respective neutron names (e.g., if 'A1' and 'A1N' exist, we will change
    //  both)
    set<string> names_to_change_set, nonN42_names_not_changed, final_n42_names;
    
    //ToDo: this next loop could be really slow on large files - should fix.
    for( const auto &m : info.measurements() )
    {
      if( !m )
        continue;
      const auto &name = m->detector_name();
      
      if( std::count(begin(dont_change_det_names), end(dont_change_det_names), name) )
      {
        if( is_n42_name(name) )
          final_n42_names.insert( name );
        else
          nonN42_names_not_changed.insert( name );
      }else if( m->gamma_counts() && m->gamma_counts()->size()>6 )
      {
        //(is_n42_name(name) ? final_n42_names : names_to_change_set).insert(name);
        if( is_n42_name(name) )
          final_n42_names.insert( name );
        else
          names_to_change_set.insert( name );
      }else
      {
        nonN42_names_not_changed.insert( name );
      }
    }//for( loop over measurements )
    
    //Some samples for a valid spectroscopic gamma detector may not have gamma
    //  data, so make sure those detectors wont be in nonN42_names_not_changed
    for( const auto &name : final_n42_names )
      nonN42_names_not_changed.erase(name);
    for( const auto &name : names_to_change_set )
      nonN42_names_not_changed.erase(name);
    
    if( names_to_change_set.empty() )
    {
      if( print_debug )
        cout << "No names to change" << endl;
      return;
    }
    
    const set<string> unchanged_n42 = final_n42_names; //for debug only
    vector<string> names_to_change( begin(names_to_change_set), end(names_to_change_set) );
    
    //boost::algorithm::is_iless gives a compile error, so for now I'll just do
    //  the stupid thing.
    //std::sort( begin(names_to_change), end(names_to_change), boost::algorithm::is_iless() );
    std::sort( begin(names_to_change), end(names_to_change), [](string lhs, string rhs) -> bool {
      std::transform( begin(lhs), end(lhs), begin(lhs),
                     [](unsigned char c){ return std::tolower(c); });
      std::transform( begin(rhs), end(rhs), begin(rhs),
                     [](unsigned char c){ return std::tolower(c); });
      return lhs < rhs;
    } );
    
    //Basing the N42 naming order off of what is used to create PCF files, eg:
    //loop over columns (2 uncompressed, or 4 compressed)  //col 1 is Aa1, col two is Ba1
    //  loop over panels (8) //Aa1, Ab1, Ac1
    //    loop over MCAs (8) //Aa1, Aa2, Aa3, etc
    
    //ToDo: This naming heuristic of when to increment column vs panel vs MCA
    //      should probably be improved on by looking at all the available
    //      example portals and making this consistent
    
    //Usually RPMs only have 2 columns (e.g., no Ca1, etc), so only use names
    //  begining with C/D if there are a lot of detectors.
    const size_t num_col = (names_to_change.size() > 16) ? 4 : 2;
    
    //
    const size_t num_panel = (names_to_change.size() > 8) ? 8 : 4;
    
    //Preffer to assign new pannels, instead of new MCAs within panels.
    const size_t num_mca = (names_to_change.size() > 32) ? 8 : 1;
    
    //Loop over non-n42-compliant original names, and assign them the first
    //  available N42 name.
    for( const auto &name : names_to_change )
    {
      //To find the first available N42 name, we will just sequentually loop
      //  over all possbile N42 names, until we find one that hasnt been used.
      //  (doing things the incredibly stupid way)
      string newprefix;
      
      auto n42_name_taken = [&](const std::string &n ) -> bool {
        for( const auto &finalname : final_n42_names )
        {
          assert( finalname.size() >= 3 );
          if( SpecUtils::iequals_ascii(finalname.substr(0,3), n) )
            return true;
        }//for
        return false;
      };//n42_name_taken lambda
      
      //A few RPMs name their detectors as "A1", "A2", "B1", "B2", which are
      //  actually valid according to N42-2006, but it seems the convention has
      //  moved so that programs expect "Aa1", "Aa2", "Ba1", "Ba2"
      if( name.size() == 2
          && ((name[0]>='A' && name[0]<='H') || (name[0]>='a' && name[0]<='h'))
          && name[1]>='1' && name[1]<='8' )
      {
        string moddedname = name.substr(0,1) + "a" + name.substr(1,1);
        moddedname[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(moddedname[0])));
        
        if( !n42_name_taken( moddedname) )
          newprefix = moddedname;
      }//if( name like "A1" || "A2" || ... )
      
      
      //For detectors with names like "DetectorInfoPan1DetG2"
      //  If only could use regex...
      if( newprefix.empty() && name.size()==21
          && SpecUtils::istarts_with(name, "DetectorInfoPan")
          && SpecUtils::iequals_ascii(name.substr(16).substr(0,4), "DetG")
          && isdigit(name[15]) && isdigit(name[20]) )
      {
        string candidate;
        candidate += ('A' + (name[15]-'1'));
        candidate += 'a';
        candidate += name[20];
        
        if( !n42_name_taken(candidate) )
          newprefix = candidate;
      }//if( detectors name like "DetectorInfoPan1DetG2" )
      
      
      for( size_t mca = 0; newprefix.empty() && mca < num_mca; ++mca )
      {
        for( size_t pannel = 0; newprefix.empty() && pannel < num_panel; ++pannel )
        {
          for( size_t col = 0; newprefix.empty() && col < num_col; ++col )
          {
            string candidate;
            candidate += 'A' + col;
            candidate += 'a' + pannel;
            candidate += '1' + mca;
            
            if( !n42_name_taken(candidate) )
              newprefix = candidate;
          }//for( loop over col )
        }//for( loop over pannel )
      }//for( loop over MCAs )
      
      
      if( newprefix.empty() )  //probably shouldnt happen, but JIC
      {
        std::cerr << "normalize_det_name_to_n42: Somehow failed to find appropriate"
        << " N42 name for detector '" << name << "' - wcjohns should fix this!" << std::endl;
        assert( !newprefix.empty() );
        continue;
      }
    
      const string newname = newprefix + (name.empty() ? string("") : (" " + name));
      
      if( print_debug )
        printf( "Changing detname='%s' to '%s'\n", name.c_str(), newname.c_str() );
      
      try
      {
        info.change_detector_name( name, newname );
      }catch( std::exception &e )
      {
        cerr << "Warning: Unexpected error changing detector name from "
             << "'" << name << "' to '" << newname << "'"
             << " - results may be suspect (" << e.what() << ")" << endl;
      }//try / catch
      
      final_n42_names.insert( newname );
      
      if( nonN42_names_not_changed.count(name+"N") || nonN42_names_not_changed.count(name+"n") )
      {
        const char n = nonN42_names_not_changed.count(name+"N") ? 'N' : 'n';
        const string newneutname = newprefix + "N " + name;
        
        try
        {
          info.change_detector_name( name+n, newneutname );
        }catch( std::exception &e )
        {
          cerr << "Warning: Unexpected error changing neutron detector name from"
               << " '" << name << "' to '" << newname << "'"
               << " - results may be suspec (" << e.what() << ")" << endl;
        }//
        
        nonN42_names_not_changed.erase( name+n );
        //printf( "Changing Neutron detname='%s' to '%s'\n", (name+n).c_str(), newneutname.c_str() );
      }
    }//for( const auto &name : orignames )
    
    if( print_debug )
    {
      if( !nonN42_names_not_changed.empty() )
      {
        string notchanged;
        for( const auto &i : nonN42_names_not_changed )
          notchanged += "'" + i + "', ";
        printf( "Didnt change det names: %s\n", notchanged.c_str() );
      }
      
      if( !unchanged_n42.empty() )
      {
        string notchanged;
        for( const auto &i : unchanged_n42 )
          notchanged += "'" + i + "', ";
        printf( "Didnt need to change det names: %s\n", notchanged.c_str() );
      }
    }//if( print_debug )
  }//void normalize_det_name_to_n42( SpecUtils::SpecFile &info )


#if defined(__APPLE__) || defined(unix) || defined(__unix) || defined(__unix__)
unsigned terminal_width()
{
  winsize ws = {};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) <= -1)
    return 80;
  unsigned w = (ws.ws_col);
  return std::max( 40u, w );
}
#elif defined(_WIN32)
unsigned terminal_width()
{
  HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
  if( handle == INVALID_HANDLE_VALUE )
    return 80;
  
  CONSOLE_SCREEN_BUFFER_INFO info;
  if( !GetConsoleScreenBufferInfo(handle, &info) )
    return 80;
  
  return unsigned(info.srWindow.Right - info.srWindow.Left);
}
#else
static_assert( 0, "Not unix and not win32?  Unsupported getting terminal width" );
#endif


}//namespace

namespace CommandLineUtil
{
  //We will allow users to specify a detector model, to help make it look like
  //  the output came from a certain detector; here is what we support so far:
  const char *OutputMetaInfoDetectorNames[]
   = { "DetectiveEX", "DetectiveDX", "uDetective",
       "DetectiveEX100", "DetectiveDX100", "GR130",
       "GR135", "identiFINDER1", "identiFINDERNG",
       "identiFINDERLaBr3"
     };
  
  enum OutputMetaInfoDetectorType
  {
    DetectiveEX, DetectiveDX, uDetective,
    DetectiveEX100, DetectiveDX100, GR130,
    GR135, identiFINDER1, identiFINDERNG,
    identiFINDERLaBr3, NumOutputMetaInfoDetectorType
  };
  
  const size_t num_OutputMetaInfoDetectorNames = sizeof(OutputMetaInfoDetectorNames) / sizeof(OutputMetaInfoDetectorNames[0]);

  //static_assert( num_OutputMetaInfoDetectorNames == NumOutputMetaInfoDetectorType, "OutputMetaInfoDetectorType not in sync with OutputMetaInfoDetectorNames" );
  
int run_command_util( const int argc, char *argv[] )
{
//  vector<string> args = split_winmain(lpCmdLine);
//  store(command_line_parser(args).options(desc).run(), vm);
  
  bool force_writing, summ_meas_for_single_out, include_all_cal_spec;
  bool sum_all_spectra, normalize_det_names, combine_all_files;
  
  bool no_background_spec, no_foreground_spec, no_intrinsic_spec;
  bool no_calibration_spec, no_unknown_spec;
  bool background_only, foreground_only, calibration_only, intrinsic_only;
  bool sum_det_per_sample, sum_samples_per_det;
  bool no_derived, only_derived;
  //bool spectra_of_likely_interest_only;
  vector<string> detector_renaimings, detectors_to_include, detectors_to_exclude;
  
#if( SpecUtils_ENABLE_D3_CHART )
  string html_to_include = "all";
#endif
  unsigned int rebin_factor;
  
  bool recursive = false;
  string inputdir, outputname, outputformatstr, calp_file;
  vector<string> inputfiles;
  float linearize_lower_energy, linearize_upper_energy;
  
  string newserialnum, newdettype, combine_files_sort;

#if( SpecUtils_INJA_TEMPLATES )
  string template_file;
  bool strip_template_blocks;
#endif
  
#if( SpecUtils_ENABLE_URI_SPECTRA )
  size_t num_uris = 1;
  vector<string> uri_options;
#endif
  
  unsigned term_width = terminal_width();
  unsigned min_description_length = ((term_width < 80u) ? term_width/2u : 40u);
  
  po::options_description cl_desc("Allowed options", term_width, min_description_length);
  cl_desc.add_options()
    ("help,h",  "produce this help message")
    ("about,a",  "produce the about message")
    ("version,v",  "print version information and exit")
    ("input,i", po::value< vector<string> >(&inputfiles),
              "input spectrum file(s)")
    ("output,o", po::value<string>(&outputname),
              "Output file or directory; if multiple input files are specified,"
              " this must be a valid existing directory.")
    ("format,f", po::value<string>(&outputformatstr),
              "Format of output spectrum file.  Must be specified when there"
              " are multiple input files, or if the output name for a single"
              " spectrum file is ambiguous.  For a single file, if this option"
              " is not specified, the output names file extension will be used"
              " to guess output format desired.\n"
              "Possible values are: \tTXT, CSV, PCF, XML (2006 N42 format),"
              " N42 (defaults to 2012 variant), 2012N42, 2006N42,"
              " CHN (binary integer variant), SPC (defaults to int variant),"
              " INTSPC, FLTSPC, SPE (IAEA format), asciispc (ASCII version of"
              " SPC), TKA, gr130 (256 channel binary format), CNF, or CALp (energy calibration only),"
#if( SpecUtils_ENABLE_D3_CHART )
              ", html (webpage plot), json (chart data in json format, equiv to '--format=html --html-output=json')"
#endif
#if( SpecUtils_ENABLE_URI_SPECTRA )
              ", URI (for e.g., embedding to QR-codes; see 'uri-option' and 'num-uri')"
#endif
     )
  
    ("force", po::value<bool>(&force_writing)->default_value(false)->implicit_value(true),
              "Forces overwriting of output file."
    )
    ("ini", po::value<string>(),
     "Filesystem path of INI file were some or all command line options are"
     " specified. Options specified on the command line are combined with"
     " options given in the INI file.  Most options can only be specified once."
    )
#if( SpecUtils_INJA_TEMPLATES )
    ("template-file", po::value<string>(&template_file),
        "Filesystem path of the template file to use (overrides --format option)."
    )
    ("strip-template-blocks", po::value<bool>(&strip_template_blocks)->default_value(false),
        "Sets whitespace stripping option in template engine."
    )
#endif
    ("inputdir", po::value<string>(&inputdir),
     "Input directory.  All files in specified directory will (try to) be"
     " converted.\n"
     "Must also specify 'output' option to point to an existing directory, as"
     " well as the 'format' option.\n"
     "May not be used with 'input' option.\n"
     "By default not recursive, see 'recursive' option."
    )
    ("recursive", po::value<bool>(&recursive)->default_value(false)->implicit_value(true),
     "When 'inputdir' is used, specifying this to be true will result in files"
     " being converted in not just the directory specified, but all of its"
     " descendant directories.  The output directory structure will mirror"
     " that of the input directory.\n"
     " Only has effect when 'inputdir' option is used.")
    ("combine-multi", po::value<bool>(&summ_meas_for_single_out)->default_value(false)->implicit_value(true),
              "For input files with multiple spectra, being saved to a output"
              " format that only allows a single spectrum, this option"
              " specifies to sum all spectra in the input file and save it"
              " as a single spectrum in the single output file.  By default a"
              " separate output file will be created for each spectrum in the"
              " input file.  This option does not effect saving input files"
              " with many spectra to a format that supports multiple spectra ("
              "which creates a single output file with multiple spectra)."
              " If the 'combine-input-files' option is being used, this summing is"
              " performed on a input file-by-file basis, before adding to the output file"
              "(e.g., each input file will have a single record in the output file).")
    ("all-calibrations", po::value<bool>(&include_all_cal_spec)->default_value(false),
              "Some input spectra will include the same gamma spectra more than"
              " once, but with different energy binning or calibration. By"
              " default Cambio will pick the calibration that makes the most"
              " sense (e.x., linear over compressed, or larger energy range"
              " over smaller, etc.) only include that in the output.  If you"
              " set this flag to true then all calibrations will be used to"
              " write the output."
    )
    ("CALp-file", po::value<string>(&calp_file),
     "Apply the energy calibration from the specified CALp file.\n\t"
     "The CALp file is applied after renaming or filtering detectors, but before summing."
    )
    ("set-serial-number", po::value<string>(&newserialnum),
     "Used to change the detector serial number written to the output file."
    )
    ("set-model", po::value<string>(&newdettype),
     "Used as a hint to alter some information in the output file to possibly"
     " make some of the meta-information of the output match what would be"
     " seen from a file from an actual detector.\n"
     "Valid models are: DetectiveEX, DetectiveDX, uDetective,"
     " DetectiveEX100, DetectiveDX100, GR130, GR135, identiFINDER1,"
     " identiFINDERNG, identiFINDERLaBr3\n"
     "What information is changed, or the exact output varies from according to"
     " input file type, output file type, input data, and detector type, so at"
     " best you should consider this some hints in the output file and not rely"
     " on results without testing the output.  If the input detector is"
     " determined to be the same as the output detector, nothing is changed."
    )
    //("spectra-of-likely-interest-only", po::value<bool>(&spectra_of_likely_interest_only)->default_value(false),
    // "Filters out all spectra, except the ones you probably want to analyze."
    // " Some formats like HPRDS N42 files come with lots of records you probably dont want to deal with."
    //)
    ("no-background-spec", po::value<bool>(&no_background_spec)->default_value(false)->implicit_value(true),
      "Removes all spectra explicitly marked, or inferred from file format, as backgrounds."
    )
    ("no-foreground-spec", po::value<bool>(&no_foreground_spec)->default_value(false)->implicit_value(true),
     "Removes all spectra explicitly marked, or inferred from file format, as foreground."
     " Input files with a single spectrum will be assumed as foreground, unless"
     " explicitly marked otherwise within the file."
    )
    ("no-intrinsic-spec", po::value<bool>(&no_intrinsic_spec)->default_value(false)->implicit_value(true),
     "Removes all spectra explicitly marked, or inferred from file format, as"
     " the detectors intrinsic radiation spectrum set at the factory."
    )
    ("no-calibration-spec", po::value<bool>(&no_calibration_spec)->default_value(false)->implicit_value(true),
     "Removes all spectra explicitly marked, or inferred from file format, as"
     " calibration spectra."
    )
    ("no-unknown-spec", po::value<bool>(&no_unknown_spec)->default_value(false)->implicit_value(true),
     "Removes all spectra that do not have their type explicitly marked or"
     " designated by the file format.  For files that have a single spectrum,"
     " unless it is explicitly marked, it will be assumed a foreground."
    )
    ("background-only", po::value<bool>(&background_only)->default_value(false)->implicit_value(true),
     "Only allow spectra marked as background to be written to the output."
    )
    ("foreground-only", po::value<bool>(&foreground_only)->default_value(false)->implicit_value(true),
     "Filter out all none-foreground spectra."
     " Input files with a single unmarked sample will be assumed as foreground."
    )
    ("calibration-only", po::value<bool>(&calibration_only)->default_value(false)->implicit_value(true),
     "Filter out all spectra not marked as calibration."
    )
    ("intrinsic-only", po::value<bool>(&intrinsic_only)->default_value(false)->implicit_value(true),
     "Filter out all spectra not marked as intrinsic."
    )
    ("derived-only", po::value<bool>(&only_derived)->default_value(false)->implicit_value(true),
     "All non-derived data will be removed.\n\t"
     "Derived data is frequently data that has been summed, background subtracted,"
     " or otherwise manipulated for use by the detectors RID algothim."
    )
    ("no-derived", po::value<bool>(&no_derived)->default_value(false)->implicit_value(true),
     "All derived data will be removed."
    )
    ( "sum-all-spectra", po::value<bool>(&sum_all_spectra)->default_value(false)->implicit_value(true),
       "Sum all spectra in each of the input files, which pass any optional"
       " filters, so that the output files contains only a single spectrum, even"
       " if the output format could support more than a single spectrum.\n\t"
       "If the 'combine-input-files' option is used, then output file will be"
       " one record sum of all input files."
    )
    ( "rebin-factor", po::value<unsigned int>(&rebin_factor)->default_value(1),
      "How many times to double the binning to reduce final number of bins."
      " 1 is no change, 2 is half as many bins as original, 3 is one fourth,"
      " 4 is one eight, and so on.  If it is asked to combine more channels than"
      " can be (ex., for 1024 channels, a rebin factor > 10), then it will be rebined"
      " down to a single channel."
    )
    ("linearize-lower-energy", po::value<float>(&linearize_lower_energy),
     "The lower energy, in keV to use to linearize the spectra.\n\t"
     "If specified, then 'linearize-upper-energy' must also be specified."
     )
    ("linearize-upper-energy", po::value<float>(&linearize_upper_energy),
     "The upper energy, in keV to use to linearize the spectra.\n\t"
     "If specified, then 'linearize-lower-energy' must also be specified."
     )
    ("rename-det", po::value<vector<string>>(&detector_renaimings)->composing(), //multitoken(),
     "Rename detector.  You can specify this option multiple times, once for"
     " each detector to rename.  The argument to this option must be formated"
     " like \"OriginalName=NewName\", where names are case sensitive.\n\t"
     "Ex., ./cambio --rename-det=VD1=Aa1 input.n42 output.pcf\n\t"
     "     ./cambio --rename-det 'Gamma 1 = Aa1' input.n42 output.pcf\n\t"
     "     (leading and trailing spaces to names are ignored when quotes used)"
    )
    ("normalize-det-names", po::value<bool>(&normalize_det_names)->default_value(false)->implicit_value(true),
     "Setting this option to true will rename detectors, if needed, to be more"
     " consistent with N42 standard by prepending detector names with prefixes"
     " such as \"Aa1\", \"Bc2\", etc.\n"
     "Currently this is done by niavely ordering alphabetically ordering the"
     " original detector names and prepending with N42 names.\n\t"
     "E.x., original detector names of \"VD1\", \"VD2\", \"VD3\" will become\n\t"
     "   \"Aa1 VD1\", \"Ba1 VD2\", \"Ca1 VD3\".\n"
     "If detectors are already named according to N42, or renamed with the"
     " 'rename-det' option, no changes will be made."
    )
  ("det-to-exclude", po::value<vector<string>>(&detectors_to_exclude)->composing(), //multitoken(),
   "Detector names to exclude from output.\n\t"
   "This filter is applied before (the optional) detector renaming, or normalizing.\n\t"
   "Detector names are case-sensitive.\n\t"
   "You can specify this option multiple times for multiple detectors.\n\t"
   "Ex., ./cambio --det-to-exclude=Aa1 input.n42 output.pcf\n\t"
   "     ./cambio --det-to-exclude 'Aa 1' input.n42 output.pcf\n\t"
  )
  ("det-to-include", po::value<vector<string>>(&detectors_to_include)->composing(), //multitoken(),
   "Detector names to include in output - non-named detectors will not be included.\n\t"
   "This filter is applied before (the optional) detector renaming, or normalizing.\n\t"
   "Detector names are case-sensitive.\n\t"
   "You can specify this option multiple times for multiple detectors.\n\t"
   "Ex., ./cambio --det-to-include=Aa1 input.n42 output.pcf\n\t"
   "     ./cambio --det-to-include 'Aa 1' input.n42 output.pcf\n\t"
  )
  ("sum-det-per-sample", po::value<bool>(&sum_det_per_sample)->default_value(false)->implicit_value(true),
   "For each sample number, sum all detectors for that sample number together.\n\t"
   "i.e. Output one spectrum for each sample, no matter how many detectors there are."
  )
  ("sum-samples-per-det", po::value<bool>(&sum_samples_per_det)->default_value(false)->implicit_value(true),
   "For each detector, sum all sample numbers together.\n\t"
   "i.e. Output one spectrum for each detector, no matter how many sample is in input."
  )
  ("combine-input-files", po::value<bool>(&combine_all_files)->default_value(false)->implicit_value(true),
   "Combines all input files, and writes a single output file."
   "  An output file name must be specified.")
  ("combine-input-files-sort", po::value<string>(&combine_files_sort)->default_value(""),
   "Can only be specified with the 'combine-input-files' option.\n\t"
   "Only possible value: 'time'.  Others may be added in future.\n\t"  //TODO: add 'count-rate-increasing', 'count-rate-decreasing'
   "By default, if not specified, records in the combined output file will be ordered\n\t"
   "according to the order of input files.")
#if( SpecUtils_ENABLE_D3_CHART )
    ("html-output", po::value<string>(&html_to_include)->default_value("all"),
     "Only applies when saving to the HTML format.  The components to include"
     " in the output file.  Options are \n\t"
       "'all': includes everything making a self contained html file \n\t"
       "'json': only the data compnents of the spectra\n\t"
       "'css': the default styling of the charts\n\t"
       "'js': the SpectrumChartD3 library\n\t"
       "'d3': the D3 javascript library\n\t"
       "'controls': the html and js for display options"
    )
#endif
#if( SpecUtils_ENABLE_URI_SPECTRA )
  ("uri-option", po::value<vector<string>>(&uri_options)->composing(), //multitoken(),
   "Only applies when saving to the URI format.\n\t"
      "Options to apply when creating the URI; potential values are\n\t"
      "NoDeflate, NoBaseXEncoding, CsvChannelData, NoZeroCompressCounts,\n\t"
      "UseUrlSafeBase64, and AsMailToUri.\n\t"
      "Multiple options may be specified, and will be combined together.\n\n"
      "(ex. '--format=uri --uri-option=AsMailToUri --uri-option=UseUrlSafeBase64')"
   )
  ("num-uri", po::value<size_t>(&num_uris)->default_value(1),
   "Only applies when saving to the URI format.\n\t"
       "The number of URIs to split the output into; if multiple\n\t"
       "spectra are being written (ex, foreground+background) then\n\t"
       "a value of 1 (default) must be used.  If a single spectrum\n\t"
       "a value from 1 to 9 can be used."
  )
#endif
  ;
  
  const char *about_msg =
"                            Cambio, Command Line Tool                          \n"
"                                      v2.1                                     \n"
"                                                                               \n"
"Please email wcjohns@sandia.gov AND cambio@sandia.gov with bug reports,        \n"
"suggestions, requests, or new file formats.                                    \n"
"Please visit https://github.com/sandialabs/cambio for updates and additional   \n"
"information.\n"
"\n"
"Copyright (C) 2014 Sandia National Laboratories, org 08126\n"
"\n"
"This library is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU Lesser General Public\n"
"License as published by the Free Software Foundation; either\n"
"version 2.1 of the License, or (at your option) any later version.\n"
"\n"
"This library is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
"Lesser General Public License for more details.\n"
 "\n"
"You should have received a copy of the GNU Lesser General Public\n"
"License along with this library; if not, write to the Free Software\n"
"Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA\n"
"\n";
  
  auto printExampleMsg = [=](){
    const string exe_name = SpecUtils::filename(argv[0]);
    
    cout << endl << "Example uses:\n"
         << "\t" << exe_name << " input.n42 output.pcf" << endl
         << "\t" << exe_name << " --format=CSV --no-background-spec=true ./path/to/input/*.n42 ./path/to/output" << endl
         << "\t" << exe_name << " --format=n42 --sum-all-spectra=1 --input input_0.pcf input_1.csv input_3.spe --output ./output/" << endl
         << "\t" << exe_name << " --sum-all-spectra true -o output.spc -i inputfile.n42" << endl
#if( SpecUtils_ENABLE_D3_CHART )
         << "\t" << exe_name << " input.n24 output.html"
#endif
         << endl;
  };
  
  po::positional_options_description p;
  p.add("input", -1);
  
  
  po::variables_map cl_vm;

  
//  instance of basic_option<charT> will be added to result,
//  with 'unrecognized' field set to 'true'. It's possible to
//  collect all unrecognized options with the 'collect_unrecognized'
//  function.
//  */
//  basic_command_line_parser& allow_unregistered();
  
  try
  {
    po::parsed_options parsed_opts
          = po::command_line_parser(argc,argv)
                  .allow_unregistered()
                  .options(cl_desc)
                  .positional(p)
                  .run();
    
    for( const auto &opt : parsed_opts.options )
    {
      if( opt.unregistered )
      {
        //Skip warning on the '--convert' flag for using the GUI EXE from command line.
        if( SpecUtils::iequals_ascii( opt.string_key, "convert") && opt.value.empty() )
          continue;
        
        cerr << "Warning, command line argument '" << opt.string_key
             << "' with ";
        
        if( opt.value.empty() )
          cerr << "no value";
        else if( opt.value.size() <= 1 )
          cerr << "value=";
        else
          cerr << "values={";
        for( size_t i = 0; i < opt.value.size(); ++i )
          cerr << (i?", ":"") << "'" << opt.value[i] << "'";
        cerr << (opt.value.size() > 1 ? "}" : "") << " was not recognized as a valid argument" << endl;
      }
    }//for( const auto &opt : parsed_opts.options )
    
    po::store( parsed_opts, cl_vm );
    po::notify( cl_vm );
    
    if( cl_vm.count("ini") )
    {
      const string ini_file = cl_vm["ini"].as<std::string>();
      try
      {
#ifdef _WIN32
        ifstream input( convert_from_utf8_to_utf16(ini_file).c_str(), ios_base::binary|ios_base::in );
#else
        ifstream input( ini_file.c_str(), ios_base::binary|ios_base::in );
#endif
        if( !input )
          throw runtime_error( "Could not open file." );
        
        //Even though we are reading from the file after the command line, it
        //  seems the command line arguments take precedent.
        //I dont understand why, and that makes me feel uneasy, so should figure
        //  out why at some point.
        
        const bool allow_unregistered = false;
        po::store( po::parse_config_file(input, cl_desc, allow_unregistered), cl_vm );
        po::notify( cl_vm );
      }catch( std::exception &e )
      {
        cerr << "Error parsing INI configuration file '" << ini_file << "': \n\t" << e.what() << endl;
        return 10;
      }
    }//if( !ini_file.empty() )
  
  }catch( std::exception &e )
  {
    cerr << "Error parsing command line arguments: " << e.what() << endl;
    cout << "\n" << about_msg << endl;
    cout << cl_desc << endl;
    printExampleMsg();
    return 1;
  }//try catch
  
  
  if( cl_vm.count("about") )
  {
    cout << about_msg << endl;
    return 0;
  }
  
  if( cl_vm.count("version") )
  {
    cout << "Cambio 2.1, Command Line Tool.\n"
         << "Compiled " << __DATE__ << ", for " << BOOST_PLATFORM << " with " << BOOST_COMPILER
         << endl;
    return 0;
  }
  
  if( cl_vm.count("help") )
  {
    cout << cl_desc << endl;
    printExampleMsg();
    return 0;
  }//if( cl_vm.count("help") )

  
  
  //Map save to extension to save to types
  map<string,SpecUtils::SaveSpectrumAsType> str_to_save_type;
  str_to_save_type["txt"]     = SpecUtils::SaveSpectrumAsType::Txt;
  str_to_save_type["csv"]     = SpecUtils::SaveSpectrumAsType::Csv;
  str_to_save_type["pcf"]     = SpecUtils::SaveSpectrumAsType::Pcf;
  str_to_save_type["xml"]     = SpecUtils::SaveSpectrumAsType::N42_2006;
  str_to_save_type["n42"]     = SpecUtils::SaveSpectrumAsType::N42_2012;
  str_to_save_type["2012n42"] = SpecUtils::SaveSpectrumAsType::N42_2012;
  str_to_save_type["2006n42"] = SpecUtils::SaveSpectrumAsType::N42_2006;
  str_to_save_type["chn"]     = SpecUtils::SaveSpectrumAsType::Chn;
  str_to_save_type["spc"]     = SpecUtils::SaveSpectrumAsType::SpcBinaryInt;
  str_to_save_type["intspc"]  = SpecUtils::SaveSpectrumAsType::SpcBinaryInt;
  str_to_save_type["fltspc"]  = SpecUtils::SaveSpectrumAsType::SpcBinaryFloat;
  
  str_to_save_type["asciispc"]  = SpecUtils::SaveSpectrumAsType::SpcAscii;
  str_to_save_type["gr130"]     = SpecUtils::SaveSpectrumAsType::ExploraniumGr130v0;
  str_to_save_type["gr135"]     = SpecUtils::SaveSpectrumAsType::ExploraniumGr135v2;
  str_to_save_type["dat"]       = SpecUtils::SaveSpectrumAsType::ExploraniumGr135v2;
  str_to_save_type["spe"]       = SpecUtils::SaveSpectrumAsType::SpeIaea;
  str_to_save_type["cnf"]       = SpecUtils::SaveSpectrumAsType::Cnf;
  str_to_save_type["tka"]       = SpecUtils::SaveSpectrumAsType::Tka;

#if( SpecUtils_ENABLE_D3_CHART )
  str_to_save_type["html"]       = SpecUtils::SaveSpectrumAsType::HtmlD3;
  str_to_save_type["json"]       = SpecUtils::SaveSpectrumAsType::HtmlD3;
  str_to_save_type["js"]         = SpecUtils::SaveSpectrumAsType::HtmlD3;
  str_to_save_type["css"]        = SpecUtils::SaveSpectrumAsType::HtmlD3;
#endif

#if( SpecUtils_ENABLE_URI_SPECTRA )
  str_to_save_type["uri"]        = SpecUtils::SaveSpectrumAsType::Uri;
#endif

  str_to_save_type["calp"]       = SpecUtils::SaveSpectrumAsType::NumTypes;
  
  //spec_exts: extensions of files that we can read.
  const string spec_exts[] = { "txt", "csv", "pcf", "xml", "n42", "chn",
    "spc", "dat", "cnf", "spe", "js", "json", "html", "css", "phd", "dat",
    "lzs", "lis", "mca", "gam", "mps", "spm", "icd", "tka", "rco", "uri"
  };
  const size_t len_spec_exts = sizeof(spec_exts)/sizeof(spec_exts[0]);
  
  
  if( !inputdir.empty() )
  {
    if( !inputfiles.empty() )
    {
      cerr << "You can not specify an input directory and seperate input files."
           << endl;
      return 13;
    }
    
    if( outputname.empty() )
    {
      cerr << "If you specify an input directory (e.g., '--inputdir'), you must"
      << " specify an output " << (combine_all_files ? "file " : "directory ")
      << "(e.g., '--output', or '-o')'" << endl;
      return 14;
    }
    
    if( !SpecUtils::is_directory(inputdir) )
    {
      cerr << "Input directory '" << inputdir << "' is not a valid directory"
           << endl;
      return 15;
    }
    
    if( !SpecUtils::is_directory(outputname) && !combine_all_files )
    {
      cerr << "Output directory '" << outputname << "' is not a valid directory"
           << endl;
      return 16;
    }
    
    if( combine_all_files && SpecUtils::is_directory(outputname) )
    {
      cerr << "Output '" << outputname << "' is a directory, but when combining spectrum file"
           << " you must specify an output file name."
           << endl;
      return 21;
    }
    
    if( recursive )
      inputfiles = SpecUtils::recursive_ls(inputdir);
    else
      inputfiles = SpecUtils::ls_files_in_directory(inputdir);
  }//if( !inputdir.empty()  )
  
  
  if( inputfiles.empty() || inputfiles[0].empty() )
  {
    cerr << "No input files specified." << endl;
    
    cout << "\n" << about_msg << endl;
    cout << cl_desc << endl;
    printExampleMsg();
    
    return 2;
  }//if( inputfiles.empty() )
  
  
  SpecUtils::trim( outputformatstr );
  SpecUtils::to_lower_ascii( outputformatstr );
  
  if( outputname.empty() )
  {
    if( combine_all_files )
    {
      cerr << "When combining input files, you must specify the output filename." << endl;
      return 22;
    }
    
    if( inputfiles.size() > 1 )
    {
      outputname = inputfiles.back();
      inputfiles.pop_back();
    }else
    {
      if( !str_to_save_type.count(outputformatstr) )
      {
        cerr << "No output file/directory specified" << endl;
        cout << cl_desc << endl;
        return 3;
      }else
      {
        outputname = inputfiles[0];
        const string::size_type pos = outputname.find_last_of('.');
        if( pos && pos != string::npos )
          outputname = outputname.substr( 0, pos );
      }
    }//if( inputfiles.size() > 1 ) / else
  }//if( outputname.empty() )
  
  
  if( (inputfiles.size() > 1) && !SpecUtils::is_directory(outputname) && !combine_all_files )
  {
    cerr << "You must specify an output directory when there are mutliple input"
         << " files  -- SpecUtils::is_directory('" << outputname << "')=" 
         << SpecUtils::is_directory(outputname) << endl;
    
    return 3;
  }//if( mutliple input files, and not a output directory )
  
  
  if( !force_writing && SpecUtils::is_file(outputname) )
  {
    cerr << "Output file ('" << outputname << "') already exists; you can force"
         << " overwriting it by using the --force option." << endl;
    return 5;
  }//if( mutliple input files, and not a output directory )
  

  if( outputformatstr.empty() && (outputname.size() > 3) 
     && ((inputfiles.size() == 1) || combine_all_files) )
  {
    const string::size_type pos = outputname.find_last_of( '.' );
    if( (pos != string::npos) && (pos < (outputname.size() - 1)) )
    {
      outputformatstr = outputname.substr( pos + 1 );
      SpecUtils::trim( outputformatstr );
      SpecUtils::to_lower_ascii( outputformatstr );
    }
  }//if( outputformatstr.empty() )
  
  
  if( outputformatstr.empty() && (inputfiles.size() > 1) && !combine_all_files )
  {
    cerr << "When multiple input files are specified, you must also specify"
         << " the output format using the --format option" << endl;
    return 4;
  }//if( outputformatstr.empty() && inputfiles.size() > 1 )
  
    
  if( str_to_save_type.find(outputformatstr) == str_to_save_type.end() )
  {
    if( outputformatstr.size() )
      cerr << "Output format type specified ('" << outputformatstr << "'), is"
           << " invalid.";
    else
      cerr << "Output format desired couldnt be guessed, use the --format flag"
           << " to specify.";
    
    cerr << "  Valid values are:\n\t";
    for( map<string,SpecUtils::SaveSpectrumAsType>::const_iterator i = str_to_save_type.begin();
         i != str_to_save_type.end(); ++i )
      cerr << i->first << ", ";
    cerr << endl;
      
    return 4;
  }//if( user specified invalid type )
  
  
  if( only_derived && no_derived )
  {
    cerr << "You can not specify both 'derived-only' and 'no-derived' to be true." << endl;
    return 40;
  }//if( only_derived && no_derived )
  
  
  SpecUtils::SaveSpectrumAsType format = str_to_save_type[outputformatstr];
  
#if( SpecUtils_INJA_TEMPLATES )
  if (!template_file.empty()) 
  {
      // Assume template format if template file is provided
      format = SpecUtils::SaveSpectrumAsType::Template;
  }
#endif
  
  assert( num_OutputMetaInfoDetectorNames == NumOutputMetaInfoDetectorType );
  OutputMetaInfoDetectorType metatype = NumOutputMetaInfoDetectorType;
  
  for( OutputMetaInfoDetectorType i = OutputMetaInfoDetectorType(0);
       i < NumOutputMetaInfoDetectorType; i = OutputMetaInfoDetectorType(i+1) )
  {
    if( SpecUtils::iequals_ascii( newdettype, OutputMetaInfoDetectorNames[i] ) )
    {
      metatype = i;
      break;
    }
  }//loop over
  
  if( newdettype.size() && metatype == NumOutputMetaInfoDetectorType )
  {
    cerr << "Detector model '" << newdettype << "' is invalid; valid models"
            " are:";
    
    for( OutputMetaInfoDetectorType i = OutputMetaInfoDetectorType(0);
        i < NumOutputMetaInfoDetectorType; i = OutputMetaInfoDetectorType(i+1) )
    {
      cerr << (i ? ", " : " ") << OutputMetaInfoDetectorNames[i];
    }
    cerr << endl;
    return 9;
  }//if( user specified invalid --model )
  
  
  //Lets see if we need to change the output file format, based on meta
  //  information hint the user wanted
  switch( metatype )
  {
    case DetectiveEX:    case DetectiveDX: case uDetective:
    case DetectiveEX100: case DetectiveDX100:
       if( outputformatstr == "spc" )
         format = SpecUtils::SaveSpectrumAsType::SpcBinaryInt;
    break;
      
    case GR130:
      if( outputformatstr == "dat" )
        format = SpecUtils::SaveSpectrumAsType::ExploraniumGr130v0;
    break;
      
    case GR135:
      if( outputformatstr == "dat" )
        format = SpecUtils::SaveSpectrumAsType::ExploraniumGr135v2;
      break;
      
    case identiFINDER1:
    case identiFINDERNG:
    case identiFINDERLaBr3:
      if( outputformatstr == "spc" )
        format = SpecUtils::SaveSpectrumAsType::SpcAscii;
      break;
      
    case NumOutputMetaInfoDetectorType:
      break;
  }//switch( metatype )
  
  
  //Make sure all the input files exist
  for( size_t i = 0; i < inputfiles.size(); ++i )
  {
    if( !SpecUtils::is_file(inputfiles[i]) )
    {
      cerr << "Input file '" << inputfiles[i] << "' doesnt exist, or cant be"
           << " accessed." << endl;
      return 6;
    }//if( input file didnt exist )
  }//for( loop over input files )
  
  
  if( background_only )
    no_foreground_spec = no_intrinsic_spec = no_calibration_spec = no_unknown_spec = true;
  if( foreground_only )
    no_background_spec = no_intrinsic_spec = no_calibration_spec = no_unknown_spec = true;
  if( calibration_only )
    no_background_spec = no_foreground_spec = no_intrinsic_spec = no_unknown_spec = true;
  if( intrinsic_only )
    no_background_spec = no_foreground_spec = no_calibration_spec = no_unknown_spec = true;
  
  string outdir, outname;
  if( SpecUtils::is_directory(outputname) )
  {
    outname = "";
    outdir = outputname;
  }else
  {
    outname = SpecUtils::filename(outputname);
    outdir = SpecUtils::parent_path(outputname);
  }
  
  
  assert( (outputformatstr == "calp") == (format == SpecUtils::SaveSpectrumAsType::NumTypes) );
  if( outputformatstr == "calp" )
  {
    // If we are making a CALp file, we can only have one input file, and we can not specify
    //  an input CALp file
    if( !calp_file.empty() )
    {
      cerr << "You can specify to output a CALp file (--format option),"
              " and specify an input CALp file (--CALp-file option)." << endl;
      return 37;
    }
    
    if( inputfiles.size() != 1 )
    {
      // We could relax this and let users make multiple CALp files at once, but I think
      //  the chance of confusion is too great, so we'll keep this option restricted.
      cerr << "When creating a CALp file, you can only specify a single input file." << endl;
      return 38;
    }
    
    if( combine_all_files )
    {
      cerr << "The '--combine-input-files' option can not be used when creating CALp file." << endl;
      return 39;
    }
  }//if( outputformatstr == "calp" )
  
  
  if( !calp_file.empty() && !SpecUtils::is_file(calp_file) )
  {
    cerr << "Specified CALp file is not a file." << endl;
    return 31;
  }//
  
  
  const bool linearize = (cl_vm.count("linearize-lower-energy")
                          || cl_vm.count("linearize-upper-energy"));
  if( linearize )
  {
    if( cl_vm.count("linearize-lower-energy") != cl_vm.count("linearize-upper-energy") )
    {
      cerr << "If you specify 'linearize-lower-energy' or 'linearize-upper-energy', you must"
           << " specify both of them." << endl;
      
      return 33;
    }//
    
    if( linearize_upper_energy <= linearize_lower_energy )
    {
      cerr << "The lower linearization energy must be less than the upper linearization energy."
      << endl;
      
      return 34;
    }//if( linearize_upper_energy <= linearize_lower_energy )
    
    if( linearize_upper_energy <= 0 )
    {
      cerr << "The upper linearization energy must be greater than 0 keV."
      << endl;
      return 35;
    }
  }//if( linearizing spectra )
  
  
  string ending = suggestedNameEnding( format );
  
  if( format == SpecUtils::SaveSpectrumAsType::NumTypes )
  {
    assert( outputformatstr == "calp" );
    ending = "CALp";
  }//if( format == SpecUtils::SaveSpectrumAsType::NumTypes )
  
  
  
 #if( SpecUtils_ENABLE_D3_CHART )
  if( format == SpecUtils::SaveSpectrumAsType::HtmlD3 )
  {
    SpecUtils::to_lower_ascii( html_to_include );
    SpecUtils::trim( html_to_include );
    
    if( outputformatstr == "json" )
      html_to_include = "json";
    
    
    if( (html_to_include != "all")
       && (html_to_include != "json")
       && (html_to_include != "css")
       && (html_to_include != "js")
       && (html_to_include != "d3")
       && (html_to_include != "controls") )
    {
      cerr << "The 'html-output' option must specify exactly one of the"
      " following: all, json, css, js, d3, controls."
      "  You specified '" << html_to_include << "'" << endl;
      
      return 10;
    }
    
    if( html_to_include == "controls" )
    {
      // When (if) we implement this look below for "control", and fix that up
      cerr << "Writing controls HTML is not supported yet - sorry" << endl;
      return 12;
    }
    
    if( html_to_include == "all" )
      ending = "html";
    else if( html_to_include == "json" )
      ending = "json";
    else if( html_to_include == "css" )
      ending = "css";
    else if( html_to_include == "js" )
      ending = "js";
    else if( html_to_include == "d3" )
      ending = "js";
  }//if( format == kD3HtmlSpectrumFile )
#endif

#if( SpecUtils_INJA_TEMPLATES )
  if (format == SpecUtils::SaveSpectrumAsType::Template) 
  {
      // We already check above that template_file is not empty
      // If we are using the template format, use the extension of the template file and not the input file
      const string::size_type pos = template_file.find_last_of('.');
      if (pos && (pos != string::npos) && (pos < (template_file.size() - 1)))
      {
          ending = template_file.substr(pos + 1);
          SpecUtils::to_lower_ascii(ending);
      }
  }
#endif

  vector<string> renamed_dets;
  map<string,string> det_renames;
  for( const string &detrename : detector_renaimings )
  {
    const auto equal_pos = detrename.find( "=" );
    if( equal_pos == string::npos )
    {
      cerr << "'rename-det' argument must be of the form \"OldName=NewName\""
           << " with the '=' characters required"
           << " (for arg '" << detrename << "')." << endl;
      return 9;
    }//
    const string from_name = SpecUtils::trim_copy( detrename.substr(0,equal_pos) );
    const string to_name = SpecUtils::trim_copy( detrename.substr(equal_pos+1) );
    det_renames[from_name] = to_name;
    renamed_dets.push_back( to_name );
  }//for( string detrename : detector_renaimings )
  
  
#if( SpecUtils_ENABLE_URI_SPECTRA )
  uint8_t uri_encode_options = 0x0;
  
  if( format != SpecUtils::SaveSpectrumAsType::Uri )
  {
    if( !uri_options.empty() )
    {
      cerr << "You can not specify any 'uri-option' options unless output format is URI." << endl;
      return 17;
    }
    
    if( num_uris != 1 )
    {
      cerr << "You can not specify the 'num-uri' option unless output format is URI." << endl;
      return 18;
    }
  }else
  {
    for( const string &opt : uri_options )
    {
      if( SpecUtils::iequals_ascii(opt, "NoDeflate") )
        uri_encode_options |= SpecUtils::EncodeOptions::NoDeflate;
      else if( SpecUtils::iequals_ascii(opt, "NoBaseXEncoding") )
        uri_encode_options |= SpecUtils::EncodeOptions::NoBaseXEncoding;
      else if( SpecUtils::iequals_ascii(opt, "CsvChannelData") )
        uri_encode_options |= SpecUtils::EncodeOptions::CsvChannelData;
      else if( SpecUtils::iequals_ascii(opt, "NoZeroCompressCounts") )
        uri_encode_options |= SpecUtils::EncodeOptions::NoZeroCompressCounts;
      else if( SpecUtils::iequals_ascii(opt, "UseUrlSafeBase64") )
        uri_encode_options |= SpecUtils::EncodeOptions::UseUrlSafeBase64;
      else if( SpecUtils::iequals_ascii(opt, "AsMailToUri") )
        uri_encode_options |= SpecUtils::EncodeOptions::AsMailToUri;
      else
      {
        cerr << "An invalid 'uri-option' option, '" << opt << "' was specified.\n\t"
        << "tValid options are: 'NoDeflate', 'NoBaseXEncoding', 'CsvChannelData',\n\t"
        << "'NoZeroCompressCounts', 'UseUrlSafeBase64', 'AsMailToUri'"
        << endl;
        return 19;
      }
    }//for( const string &opt : uri_options )
    
    if( (uri_encode_options & SpecUtils::EncodeOptions::UseUrlSafeBase64)
       && (uri_encode_options & SpecUtils::EncodeOptions::NoBaseXEncoding) )
    {
      cerr << "You can not specify 'uri-option' options 'UseUrlSafeBase64' and"
      << "'NoBaseXEncoding' together." << endl;
      return 20;
    }
  }//if( format != SpecUtils::SaveSpectrumAsType::Uri )
#endif //#if( SpecUtils_ENABLE_URI_SPECTRA )
  
  
  if( combine_all_files )
  {
    if( recursive )
    {
      cerr << "The 'recursive' option can not be combined with the 'combine-input-files' option."
      << endl;
      
      return 23;
    }
    
    if( inputfiles.size() < 2 )
    {
      cerr << "You must specify more than one input file with the 'combine-input-files' option."
      << endl;
      return 24;
    }
    
    SpecUtils::to_lower_ascii( combine_files_sort );
    SpecUtils::trim( combine_files_sort );
    
    if( !combine_files_sort.empty() && (combine_files_sort != "time")
       //&& (combine_files_sort != "count-rate-increasing")
       //&& (combine_files_sort != "count-rate-decreasing")
       )
    {
      cerr << "The 'combine-input-files-sort' option, if specified, can only take on values: "
           << "'time'"
           //<< ", 'count-rate-increasing', 'count-rate-decreasing'"
           << "." << endl;;
      
      return 25;
    }
  }else
  {
    if( !combine_files_sort.empty() )
    {
      cerr << "The 'combine-input-files-sort' option can only be specified along with the "
      << "'combine-input-files' option." << endl;
      
      return 26;
    }
  }//if( combine_all_files ) / else
  
  if( sum_det_per_sample && sum_samples_per_det )
  {
    cerr << "You can not specify both 'sum-det-per-sample' and 'sum-samples-per-det'." << endl;
    return 36;
  }//if( sum_det_per_sample && sum_samples_per_det )
  
  
 
  
  // We'll define a lambda to actually write the output file
  auto write_output_file = [
    //First we'll capture variables we wont change, by value
    force_writing, summ_meas_for_single_out,
    sum_det_per_sample, 
    sum_samples_per_det
#if( SpecUtils_ENABLE_D3_CHART )
    , html_to_include
#endif
#if( SpecUtils_ENABLE_URI_SPECTRA )
    , num_uris, uri_encode_options
#endif
#if( SpecUtils_INJA_TEMPLATES )
     , template_file, strip_template_blocks
#endif
#ifndef NDEBUG
    , outputformatstr
#endif
  ]( SpecUtils::SpecFile &info,
    const SpecUtils::SaveSpectrumAsType format,
    const string &saveto, const string &inname ) 
  -> pair<bool /*wrote all output files*/,bool /*a output file already existed, and `force_writing` was false*/ > {
    
    bool file_existed = false;
    bool opened_all_output_files = true, encoded_all_files = true;
    
    
    if( sum_det_per_sample )
    {
      const set<int> orig_samples = info.sample_numbers();
      const vector<string> orig_dets = info.detector_names();
      
      vector<shared_ptr<SpecUtils::Measurement>> keepers;
      for( const int sample : orig_samples )
      {
        vector<shared_ptr<const SpecUtils::Measurement>> sample_meass = info.sample_measurements(sample);
        if( sample_meass.size() == 1 )
        {
          auto m = make_shared<SpecUtils::Measurement>( *(sample_meass[0]) );
          m->set_detector_name( "summed" );
          keepers.push_back( m );
        }else if( sample_meass.size() > 1 )
        {
          // TODO: summing will fail if we dont have any Measurements with gamma spectra that have valid energy calibrations (e.g., all Measurements are neutrons) - we should handle this case
          try
          {
            shared_ptr<SpecUtils::Measurement> m = info.sum_measurements( {sample}, orig_dets, 0 );
            
            set<string> titles;
            bool all_background = true;
            for( auto orig : sample_meass )
            {
              const bool empty_title = orig->title().empty();
              if( !empty_title )
                titles.insert( orig->title() );
              
              all_background &= (SpecUtils::icontains( orig->title(), "Background")
                                 || (orig->source_type() == SpecUtils::SourceType::Background)
                                 || (info.passthrough() && (orig->occupied() == SpecUtils::OccupancyStatus::NotOccupied)));
            }//for( auto orig : sample_meass )
            
            assert( m );
            if( m )
            {
              m->set_detector_name( "summed" );
              m->set_sample_number( sample );
              
              if( titles.size() == 1 )
                m->set_title( *begin(titles) );
              else if( all_background )
                m->set_title( "Background" );
              else
                m->set_title( "" );
              
              keepers.push_back( m );
            }//if( m )
          }catch( std::exception & )
          {
            cerr << "Error summing records for sample " << sample << " - omitting the "
                 << sample_meass.size() << " records for this sample number." << endl;
          }//try / catch - to sum the measurements for this sample
        }//for( const int sample : orig_samples )
      }//for( const int sample : orig_samples )
      
      info.remove_measurements( info.measurements() );
      for( auto m : keepers )
        info.add_measurement( m, false );
      
      info.set_uuid( "" );
      info.cleanup_after_load( SpecUtils::SpecFile::CleanupAfterLoadFlags::DontChangeOrReorderSamples );
    }//if( sum_det_per_sample )
    
    
    if( sum_samples_per_det )
    {
      const set<int> orig_samples = info.sample_numbers();
      const vector<string> orig_dets = info.detector_names();
      
      vector<shared_ptr<SpecUtils::Measurement>> keepers;
      map<string,vector<shared_ptr<const SpecUtils::Measurement>>> detector_to_meas;
      map<string,shared_ptr<SpecUtils::Measurement>> detector_to_sum;
      
      for( const string &det : orig_dets )
      {
        vector<shared_ptr<const SpecUtils::Measurement>> &det_meas = detector_to_meas[det];
        for( const int sample : orig_samples )
        {
          auto m = info.measurement( sample, det );
          if( m )
            det_meas.push_back( m );
        }//for( const int sample : orig_samples )
        
        if( det_meas.size() == 1 )
        {
          auto m = make_shared<SpecUtils::Measurement>( *det_meas.front() );
          m->set_sample_number( 1 );
          keepers.push_back( m );
        }else if( det_meas.size() > 1 )
        {
          // TODO: summing will fail if we dont have any Measurements with gamma spectra that have valid energy calibrations (e.g., this is a neutron detector) - we should handle this case
          try
          {
            shared_ptr<SpecUtils::Measurement> m = info.sum_measurements( orig_samples, {det}, 0 );
            m->set_detector_name( det );
            m->set_sample_number( 1 );
            keepers.push_back( m );
          }catch( std::exception & )
          {
            cerr << "Error summing records for detector '" << det << "' - omitting the "
                 << orig_samples.size() << " records for this detector." << endl;
          }//try / catch - to sum the measurements for this sample
        }//if( det_meas.size() == 1 ) / else 2 or more detectors
      }//for( const string &det : orig_dets )
      
      info.remove_measurements( info.measurements() );
      for( auto m : keepers )
        info.add_measurement( m, false );
      
      info.set_uuid( "" );
      info.cleanup_after_load( SpecUtils::SpecFile::CleanupAfterLoadFlags::DontChangeOrReorderSamples );
    }//if( sum_samples_per_det )
    
    if( format == SpecUtils::SaveSpectrumAsType::Chn
       || format == SpecUtils::SaveSpectrumAsType::SpcBinaryInt
       || format == SpecUtils::SaveSpectrumAsType::SpcBinaryFloat
       || format == SpecUtils::SaveSpectrumAsType::SpcAscii
       || format == SpecUtils::SaveSpectrumAsType::SpeIaea
       || format == SpecUtils::SaveSpectrumAsType::Cnf
       || format == SpecUtils::SaveSpectrumAsType::Tka
       )
    {
      const vector< std::shared_ptr<const SpecUtils::Measurement> > meass = info.measurements();
      const std::set<int> samplenums = info.sample_numbers();
      const std::vector<int> detnums = info.detector_numbers();
      
      if( meass.size()<2 || summ_meas_for_single_out )
      {
        if( !force_writing && SpecUtils::is_file(saveto) )
        {
          cerr << "Output file '" << saveto << "' existed, and --force not"
          << " specified, not saving file" << endl;
          file_existed = true;
          return make_pair(false, file_existed);
        }//if( !force_writing && SpecUtils::is_file(savename) )
        
#ifdef _WIN32
        ofstream output( convert_from_utf8_to_utf16(saveto).c_str(), ios_base::binary | ios_base::out );
#else
        ofstream output( saveto.c_str(), ios_base::binary | ios_base::out );
#endif
        
        if( !output.is_open() )
        {
          cerr << "Failed to open output file " << saveto << endl;
          opened_all_output_files = false;
          return make_pair(false, file_existed);
        }
        
        bool wrote = false;
        std::set<int> detnumset;
        for( size_t i = 0; i < detnums.size(); ++i )
          detnumset.insert( detnums[i] );
        
        if( format == SpecUtils::SaveSpectrumAsType::Chn )
          wrote = info.write_integer_chn( output, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::SpcBinaryInt )
          wrote = info.write_binary_spc( output, SpecUtils::SpecFile::IntegerSpcType, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::SpcBinaryFloat )
          wrote = info.write_binary_spc( output, SpecUtils::SpecFile::FloatSpcType, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::SpcAscii )
          wrote = info.write_ascii_spc( output, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::SpeIaea )
          wrote = info.write_iaea_spe( output, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::Cnf )
          wrote = info.write_cnf( output, samplenums, detnumset );
        else if( format == SpecUtils::SaveSpectrumAsType::Tka )
          wrote = info.write_tka( output, samplenums, detnumset );
        else
        {
          assert( 0 );
        }
        
        if( !wrote )
        {
          encoded_all_files = false;
          cerr << "Possibly failed in writing '" << saveto << "'" << endl;
        }else
        {
          cout << "Saved '" << saveto << "'" << endl;
        }
      }else  //if( sum all measurements )
      {
        int nwroteone = 0;
        
        for( set<int>::const_iterator i = samplenums.begin(); i != samplenums.end(); ++i )
        {
          const int sample = *i;
          for( vector<int>::const_iterator j = detnums.begin(); j != detnums.end(); ++j )
          {
            const int detnum = *j;
            
            std::set<int> detnumset, samplenumset;
            detnumset.insert( detnum );
            samplenumset.insert( sample );
            
            
            const auto m = info.measurement( sample, detnum );
            if( !m )
            {
              // This detector and sample number measurement combination doesnt specify an
              //  existing measurement.
              continue;
            }
            
            string extention;
            string outname = saveto;
            
            const string::size_type pos = saveto.find_last_of( '.' );
            if( pos != string::npos )
            {
              extention = saveto.substr( pos );
              outname = saveto.substr( 0, pos );
            }//if( pos >= 0 )
            
            {
              outname += "_";
              
              char buffer[32];
              snprintf( buffer, sizeof(buffer), "%d", nwroteone );
              size_t nchar = strlen(buffer);
              while( nchar++ < 4 )  //VS2012 doesnt support %4d format flag
                outname += "0";
              outname += buffer;
            }
            
            if( extention.size() > 0 )
              outname = outname + extention;
            
            if( !force_writing && SpecUtils::is_file(outname) )
            {
              cerr << "Output file '" << outname << "' existed, and --force not"
              << " specified, not saving file" << endl;
              file_existed = true;
              continue;
            }//if( !force_writing && SpecUtils::is_file(savename) )
            
#ifdef _WIN32
            ofstream output( convert_from_utf8_to_utf16(outname).c_str(), ios_base::binary | ios_base::out );
#else
            ofstream output( outname.c_str(), ios_base::binary | ios_base::out );
#endif
            
            if( !output.is_open() )
            {
              cerr << "Failed to open output file " << outname << endl;
              opened_all_output_files = false;
              continue;
            }else
            {
              bool wrote;
              if( format == SpecUtils::SaveSpectrumAsType::Chn )
                wrote = info.write_integer_chn( output, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::SpcBinaryInt )
                wrote = info.write_binary_spc( output, SpecUtils::SpecFile::IntegerSpcType, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::SpcBinaryFloat )
                wrote = info.write_binary_spc( output, SpecUtils::SpecFile::FloatSpcType, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::SpcAscii )
                wrote = info.write_ascii_spc( output, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::SpeIaea )
                wrote = info.write_iaea_spe( output, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::Cnf )
                wrote = info.write_cnf( output, samplenumset, detnumset );
              else if( format == SpecUtils::SaveSpectrumAsType::Tka )
                wrote = info.write_tka( output, samplenumset, detnumset );
              else
                assert( 0 );
              
              
              if( !wrote )
              {
                encoded_all_files = false;
                cerr << "Possibly failed writing of '" + outname + "'" << endl;
              }else
              {
                cout << "Saved '" << outname << "'" << endl;
              }
              
              nwroteone += wrote;
            }
          }//foreach( const int detnum, detnums )
        }//foreach( const int sample, samplenums )
      }//if( we should sum all of then and then save ) / else
    }else if( format != SpecUtils::SaveSpectrumAsType::NumTypes )
    {
      // a single spectrum output format
      if( !force_writing && SpecUtils::is_file(saveto) )
      {
        cerr << "Output file '" << saveto << "' existed, and --force not"
        << " specified, not saving file" << endl;
        file_existed = true;
        return make_pair(false, file_existed);
      }//if( !force_writing && SpecUtils::is_file(savename) )
      
#ifdef _WIN32
      ofstream output( convert_from_utf8_to_utf16(saveto).c_str(), ios_base::binary | ios_base::out );
#else
      ofstream output( saveto.c_str(), ios_base::binary | ios_base::out );
#endif
      
      if( !output.is_open() )
      {
        cerr << "Failed to open output file " << saveto << endl;
        opened_all_output_files = false;
        return make_pair(false, file_existed);
      }
      
      bool wrote = false;
      switch( format )
      {
        case SpecUtils::SaveSpectrumAsType::Txt:
          wrote = info.write_txt( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::Csv:
          wrote = info.write_csv( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::Pcf:
          wrote = info.write_pcf( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::N42_2006:
          wrote = info.write_2006_N42( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::N42_2012:
          wrote = info.write_2012_N42( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::ExploraniumGr130v0:
          wrote = info.write_binary_exploranium_gr130v0( output );
          break;
          
        case SpecUtils::SaveSpectrumAsType::ExploraniumGr135v2:
          wrote = info.write_binary_exploranium_gr135v2( output );
          break;
          
#if( SpecUtils_ENABLE_D3_CHART )
        case SpecUtils::SaveSpectrumAsType::HtmlD3:
        {
          const vector<std::shared_ptr<const SpecUtils::Measurement> > measurements = info.measurements();
          
          //Should probably look to see if this is an obvious case where we
          //  should show the foreground/background on the same chart.
          if( SpecUtils::iequals_ascii( html_to_include, "json" ) )
          {
            output << "[\n\t";
            
#if( WRITE_JSON_META_INFO )
            add_file_meta_info_to_json( output, info );
#endif
            
            for( size_t i = 0; i < measurements.size(); ++i )
            {
              const auto &meas = *measurements[i];
              
#if( WRITE_JSON_META_INFO )
              //output meta-information on the spectrum level (there may be multiple spectra in a single file)
              //  not implemented at all yet; should probably be done inside D3SpectrumExport::write_spectrum_data_js(...)
              //meas.has_gps_info() { meas.position_time(), meas.latitude(), meas.longitude()}
              //meas.contained_neutron()
              //meas.remarks()
              //meas.source_type()
#endif
              if( i != 0 )
                output << ",\n\t";
              
              D3SpectrumExport::D3SpectrumOptions options;
              options.line_color = "black";
              options.display_scale_factor = 1.0;
              wrote = D3SpectrumExport::write_spectrum_data_js( output, meas, options, 0, -1 );
            }//for( loop over measurements )
            
            output << "\n]" << endl;
          }else if( SpecUtils::iequals_ascii( html_to_include, "css" ) )
          {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
            output << D3SpectrumExport::spectrum_char_d3_css() << endl;
#else
            const char *incssname = D3SpectrumExport::spectrum_char_d3_css_filename();
#ifdef _WIN32
            ofstream incss( convert_from_utf8_to_utf16(incssname).c_str(), ios::in | ios::binary );
#else
            ofstream incss( incssname.c_str(), ios::in | ios::binary );
#endif
            if( !incss.is_open() )
            {
              cerr << "Could not open the input CSS file '" << incssname << "'" << endl;
              return make_pair(false, file_existed);
            }
            output << incss.rdbuf();
#endif
            break;
            wrote = output.good();
          }else if( SpecUtils::iequals_ascii( html_to_include, "js" ) )
          {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
            output << D3SpectrumExport::spectrum_chart_d3_js() << endl;
#else
            const char *incssname = D3SpectrumExport::spectrum_chart_d3_js_filename();
#ifdef _WIN32
            ofstream incss( convert_from_utf8_to_utf16(incssname).c_str(), ios::in | ios::binary );
#else
            ofstream incss( incssname.c_str(), ios::in | ios::binary );
#endif
            if( !incss.is_open() )
            {
              cerr << "Could not open the input SpectrumChartD3 file '" << incssname << "'" << endl;
              return make_pair(false, file_existed);
            }
            output << incss.rdbuf();
#endif
            wrote = output.good();
            break;
          }else if( SpecUtils::iequals_ascii( html_to_include, "d3" ) )
          {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
            output << D3SpectrumExport::d3_js() << endl;
#else
            const char *incssname = D3SpectrumExport::d3_js_filename();
#ifdef _WIN32
            ofstream incss( convert_from_utf8_to_utf16(incssname).c_str(), ios::in | ios::binary );
#else
            ofstream incss( incssname.c_str(), ios::in | ios::binary );
#endif
            if( !incss.is_open() )
            {
              cerr << "Could not open the input D3.js file '" << incssname << "'" << endl;
              return make_pair(false, file_existed);
            }
            output << incss.rdbuf();
#endif
            wrote = output.good();
            break;
          }else if( SpecUtils::iequals_ascii( html_to_include, "controls" ) )
          {
            cerr << "Writing controls HTML is not supported yet - sorry" << endl;
            assert( 0 );
            exit( 12 );
          }else if( SpecUtils::iequals_ascii( html_to_include, "all" ) )
          {
            using namespace D3SpectrumExport;
            D3SpectrumChartOptions fileopts;
            D3SpectrumExport::D3SpectrumOptions specopts;
            specopts.line_color = "black";
            specopts.display_scale_factor = 1.0;
            
            fileopts.m_title = SpecUtils::filename( inname );
            fileopts.m_useLogYAxis = true;
            fileopts.m_showVerticalGridLines = false;
            fileopts.m_showHorizontalGridLines = false;
            fileopts.m_legendEnabled = true;
            fileopts.m_compactXAxis = false;
            fileopts.m_showPeakUserLabels = false;
            fileopts.m_showPeakEnergyLabels = false;
            fileopts.m_showPeakNuclideLabels = false;
            fileopts.m_showPeakNuclideEnergyLabels = false;
            fileopts.m_showEscapePeakMarker = false;
            fileopts.m_showComptonPeakMarker = false;
            fileopts.m_showComptonEdgeMarker = false;
            fileopts.m_showSumPeakMarker = false;
            //fileopts.m_xMin = ;
            //fileopts.m_xMax = ; // energy range user is zoomed into
            
            std::map<std::string,std::string> m_reference_lines_json;
            
            
            const char *endline = "\r\n";
            
            html_page_header( output, fileopts.m_title );
            
            output << "<body>" << endline;
            
            for( size_t i = 0; i < measurements.size(); ++i )
            {
              std::shared_ptr<const SpecUtils::Measurement> m = measurements[i];
              const string div_id = "chart" + std::to_string(i);
              
              fileopts.m_dataTitle = m->title();
              if( fileopts.m_dataTitle.empty() )
              {
                if( m->detector_name() != "" )
                  fileopts.m_dataTitle = "Det " + m->detector_name() + " ";
                fileopts.m_dataTitle += "Sample " + to_string(m->sample_number());
              }
              
              vector< pair<const SpecUtils::Measurement *,D3SpectrumOptions> > htmlinput;
              htmlinput.push_back( std::pair<const SpecUtils::Measurement *,D3SpectrumOptions>(m.get(),specopts) );
              
              output << "<div id=\"" << div_id << "\" class=\"chart\" oncontextmenu=\"return false;\";></div>" << endline;  // Adding the main chart div
              
              
              output << "<script>" << endline;
              write_js_for_chart( output, div_id, fileopts.m_dataTitle, fileopts.m_xAxisTitle, fileopts.m_yAxisTitle );
              write_and_set_data_for_chart( output, div_id, htmlinput );
              
              output <<
                "const resizeChart_" << div_id << " = function(){\n"
                "  let height = window.innerHeight;\n"
                "  let width = window.innerWidth; \n"
                "  let el = spec_chart_" << div_id << ".chart;\n"
                "  el.style.width = (width - 40) + \"px\";\n"
                "  el.style.height = Math.max(250, Math.min(0.4*width,height-175)) + \"px\";\n"
                "  el.style.marginLeft = \"20px\";\n"
                "  el.style.marginRight = \"20px\";\n"
                "  spec_chart_" << div_id << ".handleResize();\n"
                "};\n"
                "resizeChart_" << div_id << "();\n"
                "window.addEventListener('resize', resizeChart_" << div_id << ");\n";
              //output << "window.addEventListener('resize',function(){spec_chart_" << div_id << ".handleResize();});" << endline;
              
              write_set_options_for_chart( output, div_id, fileopts );
              output << "</script>" << endline;
              
              write_html_display_options_for_chart( output, div_id, fileopts );
            }
            
            output << "</body>" << endline;
            output << "</html>" << endline;
            
            wrote = output.good();
          }else
          {
            // We should have already caught this above in our initial checks
            const string msg = "The 'html-output' value of '" + html_to_include + "'"
            ", should not have made beyond initial checks - this is an internal logic error,"
            " and should be corrected.";
            
            cerr << msg << endl;
            
            assert( 0 );
            exit( 10 );
            throw std::logic_error( msg );
            return make_pair(false, file_existed);
          }
          
          break;
        }//case SpecUtils::SaveSpectrumAsType::HtmlD3:
#endif  //#if( SpecUtils_ENABLE_D3_CHART )
          
#if( SpecUtils_INJA_TEMPLATES )
        case SpecUtils::SaveSpectrumAsType::Template:
          wrote = info.write_template(output, template_file, strip_template_blocks);
          break;
#endif
          
#if( SpecUtils_ENABLE_URI_SPECTRA )
        case SpecUtils::SaveSpectrumAsType::Uri:
        {
          if( num_uris == 0 )
            throw runtime_error( "You must specify to use at least one URI." );
          
          if( num_uris > 9 )
            throw runtime_error( "You may specify a maximum of 9 URIs." );
          
          // TODO: account for neutron records being separate from gammas
          const size_t num_meas = info.num_measurements();
          if( (num_meas > 1) && (num_uris > 1) )
            throw runtime_error( "There are " + std::to_string(num_meas) + " measurements, but"
                                " you specified to write to " + std::to_string(num_uris)
                                + " URIs.  Multiple measurements may only be written to a"
                                " single URI." );
          
          info.write_uri( output, num_uris, uri_encode_options );
          
          wrote = output.good();
          
          break;
        }//case SpecUtils::SaveSpectrumAsType::Uri:
#endif //SpecUtils_ENABLE_URI_SPECTRA
          
        case SpecUtils::SaveSpectrumAsType::Chn:
        case SpecUtils::SaveSpectrumAsType::SpcBinaryInt:
        case SpecUtils::SaveSpectrumAsType::SpcBinaryFloat:
        case SpecUtils::SaveSpectrumAsType::NumTypes:
        case SpecUtils::SaveSpectrumAsType::SpeIaea:
        case SpecUtils::SaveSpectrumAsType::SpcAscii:
        case SpecUtils::SaveSpectrumAsType::Cnf:
        case SpecUtils::SaveSpectrumAsType::Tka:
          assert( 0 );
          break;
      }//switch( format )
      
      if( !wrote )
      {
        encoded_all_files = false;
        cerr << "Possibly failed write of '" << saveto << "'" << endl;
      }else
      {
        cout << "Saved '" << saveto << "'" << endl;
      }
    }else //if( we are writing a CALp file )
    {
      assert( (outputformatstr == "calp") == (format == SpecUtils::SaveSpectrumAsType::NumTypes) );
    
      // a single spectrum output format
      if( !force_writing && SpecUtils::is_file(saveto) )
      {
        cerr << "Output file '" << saveto << "' existed, and --force not"
        << " specified, not saving file" << endl;
        file_existed = true;
        return make_pair(false, file_existed);
      }//if( !force_writing && SpecUtils::is_file(savename) )
      
      size_t num_written = 0;
      stringstream calp_contents;
    
      set<string> dets_so_far;
      
      //Note that disp_detectors has both gamma and neutron detectors, but we only care about gamma
      const vector<string> &gamma_detectors = info.gamma_detector_names();
      const vector<string> &all_detectors = info.detector_names();
      const vector<string> &neut_dets = info.neutron_detector_names();
      
      for( const int sample : info.sample_numbers() )
      {
        for( const string &det : all_detectors )
        {
          if( dets_so_far.count(det) )
            continue;
          
          const shared_ptr<const SpecUtils::Measurement> m = info.measurement( sample, det );
          shared_ptr<const SpecUtils::EnergyCalibration> cal = m ? m->energy_calibration() : nullptr;
          
          if( !cal || !cal->valid() || (cal->num_channels() < 3) )
          {
            // We'll assume that a detector that only has neutrons, will always only have neutrons...
            //  TODO: this may not actually be that good of an assumptions; re-evaluate later.
            if( std::find(begin(neut_dets), end(neut_dets), det) != end(neut_dets) )
              dets_so_far.insert( det );
            
            continue;
          }//if( energy cal is not valid )
          
          // Dont write the detector name if its unambiguous
          const string detname = (gamma_detectors.size() == 1) ? string() : det;
          
          if( SpecUtils::write_CALp_file(calp_contents, cal, detname) )
          {
            num_written += 1;
            dets_so_far.insert( det );
          }else
          {
            cerr << "Error writing CALp for detector '" << detname << "'" << endl;
          }
        }//for( const string det : all_detectors )
        
        if( all_detectors.size() == dets_so_far.size() )
          break;
      }//for( const int sample : m_interspec->displayedSamples(type) )
      
      
      if( num_written == 0 )
      {
        cerr << "Failed to create CALp file contents for " << saveto << endl;
        opened_all_output_files = false;
        return make_pair(false, file_existed);
      }//if( num_written == 0 )
      
#ifdef _WIN32
      ofstream output( convert_from_utf8_to_utf16(saveto).c_str(), ios_base::binary | ios_base::out );
#else
      ofstream output( saveto.c_str(), ios_base::binary | ios_base::out );
#endif
      
      if( !output.is_open() )
      {
        cerr << "Failed to open output file " << saveto << endl;
        opened_all_output_files = false;
        return make_pair(false, file_existed);
      }
      
      output << calp_contents.str() << endl;
    }//if( a single spectrum output format )
    
    const bool full_success = (opened_all_output_files && encoded_all_files);
    return make_pair(full_success, file_existed);
  };//write_output_file lamdba
  
  
  vector<SpecUtils::SpecFile> files_to_combine; // entries only added if 'combine-input-files' option (see bool `combine_all_files`) is specified.
  
  bool parsed_all = true, input_didnt_exist = false,
       file_existed = false, wrote_all = true;
  
  for( size_t i = 0; i < inputfiles.size(); ++i )
  {
    try
    {
      if( !SpecUtils::is_file(inputfiles[i]) )
      {
        input_didnt_exist = true;
        cerr << "Input file '" << inputfiles[i] << "' doesnt exist, or cant be"
             << " accessed." << endl;
        continue;
      }//if( input file didnt exist )
    
      SpecUtils::SpecFile info;
    
      const string inname = inputfiles[i];
      const bool loaded = info.load_file( inname, SpecUtils::ParserType::Auto, inname );
      if( !loaded )
      {
        cerr << "Failed to parse '" << inname << "'" << endl;
        parsed_all = false;
        continue;
      }//if( !loaded )
      
      const set<string> cals = info.energy_cal_variants();
      
      if( cals.size() > 1 && !include_all_cal_spec )
      {
        //Calibrations I've seen:
        //"CmpEnCal", and "LinEnCal"
        //"2.5MeV" vs "9MeV"
        //{"EnergyCalibration", "LowEnergyCalibration", vs "FullEnergyCalibration"}
        
        string prefered_variant;
        for( const auto str : cals )
        {
          if( SpecUtils::icontains( str, "Lin") )
            prefered_variant = str;
          //else if( SpecUtils::iequals_ascii( str, "EnergyCalibration") )
          //  prefered_variant = str;
        }//for( const auto &str : cals )
        
        if( prefered_variant.empty() )
        {
          auto getMev = []( string str ) -> double {
            SpecUtils::to_lower_ascii(str);
            size_t pos = str.find("mev");
            if( pos == string::npos )
              return -999.9;
            str = str.substr(0,pos);
            SpecUtils::trim( str );
            for( size_t index = str.size(); index > 0; --index )
            {
              const char c = str[index-1];
              if( !isdigit(c) && c != '.' && c!=',' )
              {
                str = str.substr(index);
                break;
              }
            }
            
            double val;
            if( stringstream(str) >> val )
              return val;
            
            return -999.9;
          };
          
          double maxenergy = -999.9;
          for( const auto str : cals )
          {
            const double energy = getMev(str);
            if( (energy > 0.0) && (energy > maxenergy) )
            {
              maxenergy = energy;
              prefered_variant = str;
            }
          }//for( const auto str : cals )
          
        }//if( prefered_variant.empty() )
        
        if( prefered_variant.size() )
        {
          info.keep_energy_cal_variants( {prefered_variant} );
        }else
        {
          cerr << "Couldn't identify a preferred energy variant out of {";
          int calnum = 0;
          for( const auto str : cals )
            cerr << (calnum++ ? ", " : "") << str;
          cerr << "} - so including all energy calibrations" << endl;
        }
      }//if( more than one calibration present, and we only want one )
      
      
      if( !detectors_to_exclude.empty() || !detectors_to_include.empty() )
      {
        vector<string> keeper_names = info.detector_names();
        
        for( const string &det : detectors_to_exclude )
        {
          const auto pos = std::find( begin(keeper_names), end(keeper_names), det );
          if( pos == end(keeper_names) )
            cerr << "Warning: input file '" << inname
                 << "', does not contain a detector named '" << det << "' - ignoring." << endl;
          else
            keeper_names.erase( pos );
        }//for( const string det : detectors_to_exclude )
        
        if( !detectors_to_include.empty() )
        {
          vector<string> keepers;
          for( const string &det : detectors_to_include )
          {
            const auto pos = std::find( begin(keeper_names), end(keeper_names), det );
            if( pos == end(keeper_names) )
              cerr << "Warning: input file '" << inname
                   << "', does not contain a detector named '" << det << "' - ignoring." << endl;
            else
              keepers.push_back( det );
          }//for( const string &det : detectors_to_include )
          
          keeper_names.swap( keepers );
        }//if( !detectors_to_include.empty() )
        
        
        if( keeper_names.empty() )
        {
          cerr << "Warning: there were no detectors left, after filtering for input file '"
          << inname << "' - skipping file." << endl;
          
          continue;
        }//if( keeper_names.empty() )
        
        
        for( const auto &m : info.measurements() )
        {
          const auto pos = std::find( begin(keeper_names), end(keeper_names), m->detector_name() );
          if( pos == end(keeper_names) )
            info.remove_measurement( m, false );
        }
        
        try
        {
          info.set_uuid( "" );
          info.cleanup_after_load();
        }catch( std::exception &e )
        {
          cerr << "Error removing spectra from '" << inname << "': " << e.what() << " -- skipping file." << endl;
          continue;
        }//try / catch
      }//if( !detectors_to_exclude.empty() || !detectors_to_include.empty() )
      
      const set<int> prefilter_samples = info.sample_numbers();
      
      auto remove_type = [&info,inname,prefilter_samples]( SpecUtils::SourceType type ){
        int nremoved = 0;
        vector<shared_ptr<const SpecUtils::Measurement>> meass = info.measurements();
        for( shared_ptr<const SpecUtils::Measurement> &m : meass )
        {
          SpecUtils::SourceType record_type = m->source_type();
          
          if( record_type == SpecUtils::SourceType::Unknown
             && prefilter_samples.size()==1 )
            record_type = SpecUtils::SourceType::Foreground;
          
          if( record_type == type )
          {
            ++nremoved;
            info.remove_measurement( m, false );
          }
        }//for( loop over measurements )
        
        if( nremoved )
        {
          try
          {
            info.set_uuid( "" );
            info.cleanup_after_load();
          }catch( std::exception &e )
          {
            cerr << "Error removing spectra from '" << inname << "': " << e.what() << endl;
          }//try / catch
        }//if( nremoved )
      };//remove_type(...)

      if( no_background_spec )
        remove_type( SpecUtils::SourceType::Background );
      
      if( no_foreground_spec )
        remove_type( SpecUtils::SourceType::Foreground );
      
      if( no_intrinsic_spec )
        remove_type( SpecUtils::SourceType::IntrinsicActivity );
      
      if( no_calibration_spec )
        remove_type( SpecUtils::SourceType::Calibration );
      
      if( no_unknown_spec )
        remove_type( SpecUtils::SourceType::Unknown );
      
      
      auto remove_derived_or_nonderived = [&info,inname,prefilter_samples]( const bool remove_derived ){
        int nremoved = 0;
        vector<shared_ptr<const SpecUtils::Measurement>> meass = info.measurements();
        for( shared_ptr<const SpecUtils::Measurement> &m : meass )
        {
          const bool is_derived = m->derived_data_properties();
          if( remove_derived == is_derived )
          {
            ++nremoved;
            info.remove_measurement( m, false );
          }
        }//for( loop over measurements )
        
        if( nremoved )
        {
          try
          {
            info.set_uuid( "" );
            info.cleanup_after_load();
          }catch( std::exception &e )
          {
            cerr << "Error removing (non)-derived spectra from '" << inname << "': " << e.what() << endl;
          }//try / catch
        }//if( nremoved )
      };//remove_type(...)
      
      if( only_derived )
        remove_derived_or_nonderived( false );
      
      if( no_derived )
        remove_derived_or_nonderived( true );
      
      if( sum_all_spectra )
      {
        const set<int> sample_num = info.sample_numbers();
        const std::vector<string> det_names = info.detector_names();
        shared_ptr<SpecUtils::Measurement> summed_meas = info.sum_measurements( sample_num, det_names, nullptr );
        vector<shared_ptr<const SpecUtils::Measurement>> meass = info.measurements();
        for( shared_ptr<const SpecUtils::Measurement> &m : meass )
          info.remove_measurement( m, false );
        info.add_measurement( summed_meas, true );
        try
        {
          info.set_uuid( "" );
          info.cleanup_after_load();
        }catch( std::exception &e )
        {
          cerr << "Error summing all spectra from '" << inputfiles[i] << "': "
               << e.what() << "\n\tSkipping file." << endl;
          continue;
        }//try / catch
      }//if( sum_all_spectra )
      
      for( const auto from_to : det_renames )
      {
        //printf( "By explicit request changing detname='%s' to '%s'\n", from_to.first.c_str(), from_to.second.c_str() );
        try
        {
          info.change_detector_name( from_to.first, from_to.second );
        }catch( std::exception & )
        {
          cerr << "Warning: no detector named '" << from_to.first
               << "' to rename to '" << from_to.second << "'" << endl;
        }
      }//for( const auto from_to : det_renames )
      
      if( normalize_det_names )
        normalize_det_name_to_n42( info, renamed_dets );
      
      if( !calp_file.empty() )
      {
#ifdef _WIN32
        ifstream calp_strm( convert_from_utf8_to_utf16(calp_file).c_str(), ios_base::binary|ios_base::in );
#else
        ifstream calp_strm( calp_file.c_str(), ios_base::binary|ios_base::in );
#endif
        
        try
        {
          info.set_energy_calibration_from_CALp_file( calp_strm );
        }catch( std::exception &e )
        {
          cerr << "Error applying CALp file ('" << calp_file << "') to '" << inputfiles[i] << "': "
               << e.what() << endl;
          return 32;
        }//try / catch
      }//if( !calp_file.empty() )
      
      
      if( rebin_factor > 1 )
      {
        set<size_t> nchannels;
        for( const auto &m : info.measurements() )
          nchannels.insert( m->num_gamma_channels() );
        
        for( const size_t nchann : nchannels )
        {
          if( nchann < 2 )
            continue;
          
          const size_t ncombine = static_cast<size_t>( std::pow( 2, (rebin_factor-1) ) );
          if( (nchann % ncombine) != 0 )
          {
            cerr << "Not rebinning spectra with " << nchann << " as " << nchann
                 << "%" << ncombine << "=" << (nchann % ncombine) << endl;
            continue;
          }
          
          info.combine_gamma_channels( ncombine, nchann );
        }//for( const size_t nchann : nchannels )
      }//if( rebin_factor > 1 )
      
      
      if( linearize )
      {
        // We'll re-use energy calibration between measurements that have the same number of channels
        map<size_t,shared_ptr<SpecUtils::EnergyCalibration>> energy_cals;
        for( shared_ptr<const SpecUtils::Measurement> m : info.measurements() )
        {
          assert( m );
          const size_t nchannel = m->num_gamma_channels();
          
          if( (nchannel >= SpecUtils::EnergyCalibration::sm_min_channels)
             && m->energy_calibration()
             && m->energy_calibration()->valid() )
          {
            auto cal_pos = energy_cals.find( nchannel );
            if( cal_pos == end(energy_cals) )
            {
              auto cal = make_shared<SpecUtils::EnergyCalibration>();
              vector<float> coeffs{ linearize_lower_energy, linearize_upper_energy - linearize_lower_energy };
              cal->set_full_range_fraction( nchannel, coeffs, {} );
              cal_pos = energy_cals.insert( make_pair(nchannel, cal) ).first;
            }//if( cal_pos == end(energy_cals) )
            
            const shared_ptr<SpecUtils::EnergyCalibration> &cal = cal_pos->second;
            assert( cal && cal->valid() );
            
            info.rebin_measurement( cal, m );
          }//if( we can linearize this spectrum )
        }//for( shared_ptr<const SpecUtils::Measurement> m : info.measurements() )
      }//if( linearize )
      
      string savename = outname;
      
      if( savename.empty() )
      {
        savename = SpecUtils::filename(inname);
        const string::size_type pos = savename.find_last_of( '.' );
        if( pos && (pos != string::npos) && (pos < (savename.size()-1)) )
        {
          string ext = savename.substr(pos+1);
          SpecUtils::to_lower_ascii( ext );
        
          if( std::count(spec_exts, spec_exts+len_spec_exts, ext) )
            savename = savename.substr( 0, pos );
        }//if( pos != string::npos )
      }//if( savename.empty() )
    
      const string::size_type pos = savename.find_last_of( '.' );
      if( pos && (pos != string::npos) && (pos < (savename.size()-1)) )
      {
        const string ext = savename.substr(pos+1);
        if( !SpecUtils::iequals_ascii( ext, ending )  )
          savename += "." + ending;
      }else
      {
        savename += "." + ending;
      }
    
      string saveto = SpecUtils::append_path( outdir, savename );
    
      if( !inputdir.empty() && recursive )
      {
        assert( outputname == outdir );
        //Need to get relative path difference between inputdir and inputfiles[i]
        //and then make that hierarchy of directories, if it doesnt already exist.
        //SpecUtils::create_directory(const std::string &name)
        
        const string reldir = SpecUtils::fs_relative( inputdir, SpecUtils::parent_path(inputfiles[i]) );
        
        //ToDo: implement something like SpecUtils:recursive_create_directory(...)
        //      rather than this huge hack to recursively make directories.
        
        string tmpdirstr = reldir;
        std::deque<string> dirstomake;
        while( !tmpdirstr.empty() && tmpdirstr!= "." )
        {
          string fullpath = SpecUtils::append_path( outdir, tmpdirstr );
          string parent = SpecUtils::parent_path( fullpath );
          string leaf = SpecUtils::fs_relative( parent, fullpath );
          dirstomake.push_front( leaf );
          tmpdirstr = SpecUtils::fs_relative( outdir, parent );
        }
        
        tmpdirstr = outdir;
        for( const auto leaf : dirstomake )
        {
          tmpdirstr = SpecUtils::append_path( tmpdirstr, leaf );
          if( !SpecUtils::is_directory(tmpdirstr) )
          {
            //cout << "Will make directory '" << tmpdirstr << "'" << endl;
            SpecUtils::create_directory(tmpdirstr);
          }else
          {
            //cout << "Dont need to make directory '" << tmpdirstr << "'" << endl;
          }
        }//for( const auto leaf : dirstomake )
        
        const string fulloutdir = SpecUtils::append_path( outdir, reldir );
        saveto = SpecUtils::append_path( fulloutdir, savename );
        
        //cout << "Continuing rather than writing" << endl;
        //continue; //debug
      }//if( !inputdir.empty() && recursive )
      
      
      
      if( saveto == inname )
      {
        cerr << "Output file '" << saveto << "' identical to input file name,"
             << " not saving file" << endl;
        file_existed = true;
        continue;
      }
    
      vector< std::shared_ptr<const SpecUtils::Measurement> > meass = info.measurements();
      
      
      
      bool containted_netruon = false;
      for( size_t i = 0; i < meass.size(); ++i )
        containted_netruon |= meass[i]->contained_neutron();
      
      switch( metatype )
      {
        case DetectiveEX:
          for( size_t i = 0; i < meass.size(); ++i )
            if( !meass[i]->contained_neutron() )
              info.set_contained_neutrons( true, 0.0, meass[i], 0.0f );
              info.set_contained_neutrons( true, 0.0, meass[i], -1.0f );

          if( info.detector_type() == SpecUtils::DetectorType::DetectiveEx )
            break;
          
          info.set_instrument_id( "240 Detective-EX" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveEX" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::DetectiveEx );
        break;
          
        case DetectiveDX:
          for( size_t i = 0; i < meass.size(); ++i )
            info.set_contained_neutrons( false, 0.0f, meass[i], 0.0f );
            info.set_contained_neutrons( false, 0.0f, meass[i], -1.0f );
          
          if( info.detector_type() == SpecUtils::DetectorType::DetectiveEx )
            break;
          
          info.set_instrument_id( "240 Detective-DX" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveDX" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::DetectiveEx );
        break;
          
        case uDetective:
          if( info.detector_type() == SpecUtils::DetectorType::MicroDetective )
            break;
          
          info.set_instrument_id( "7258 MicroDetective" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "MicroDetective" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::DetectiveEx );
        break;
          
        case DetectiveEX100:
          for( size_t i = 0; i < meass.size(); ++i )
            if( !meass[i]->contained_neutron() )
              info.set_contained_neutrons( true, 0.0, meass[i], 0.0f );
              info.set_contained_neutrons( true, 0.0, meass[i], -1.0f );

          if( info.detector_type() == SpecUtils::DetectorType::DetectiveEx100 )
            break;
          
          info.set_instrument_id( "7000 Detective-EX100" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveEX100" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::DetectiveEx100 );
          break;
          
        case DetectiveDX100:
          for( size_t i = 0; i < meass.size(); ++i )
            info.set_contained_neutrons( false, 0.0f, meass[i], 0.0f );
            info.set_contained_neutrons( false, 0.0f, meass[i], -1.0f );
          
          if( info.detector_type() == SpecUtils::DetectorType::DetectiveEx100 )
            break;
          
          info.set_instrument_id( "7000 Detective-DX100" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveDX100" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::DetectiveEx100 );
        break;
          
        case GR130:
          break;
          
        case GR135:
          break;
          
        case identiFINDER1:
          info.set_manufacturer( "FLIR" );
          info.set_instrument_model( "identiFINDER" );
          info.set_detector_type( SpecUtils::DetectorType::IdentiFinder );
          info.set_instrument_type( "RadionuclideIdentifier" );
          break;
          
        case identiFINDERNG:
          if( containted_netruon )
            info.set_instrument_model( "identiFINDER 2 NGH" );
          else
            info.set_instrument_model( "identiFINDER 2 NG" );
          info.set_manufacturer( "FLIR" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::IdentiFinderNG );
          break;
          
        case identiFINDERLaBr3:
          if( containted_netruon )
            info.set_instrument_model( "identiFINDER 2 LGH" );
          else
            info.set_instrument_model( "identiFINDER 2 LG" );
          info.set_manufacturer( "FLIR" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( SpecUtils::DetectorType::IdentiFinderLaBr3 );
          break;
          
        case NumOutputMetaInfoDetectorType:
          break;
      }//switch( metatype )
      
      
      if( cl_vm.count("set-serial-number") )
        info.set_instrument_id( newserialnum );

    
      if( info.measurements().empty() )
      {
        wrote_all = false;
        //encoded_all_files = false;
        cerr << "After filtering, '" << inname << "' had no spectra left to write - skipping." << endl;
        continue;
      }//if( we filteres out all the measuremnts )
      
      if( combine_all_files )
      {
        if( summ_meas_for_single_out && (info.num_measurements() > 1) )
        {
          shared_ptr<SpecUtils::Measurement> summed_meas = info.sum_measurements( info.sample_numbers(),
                                                                      info.detector_names(), nullptr );
          for( shared_ptr<const SpecUtils::Measurement> &m : info.measurements() )
            info.remove_measurement( m, false );
          info.add_measurement( summed_meas, true );
          try
          {
            info.set_uuid( "" );
            info.cleanup_after_load();
          }catch( std::exception &e )
          {
            cerr << "Error summing all spectra in file '" << inputfiles[i] << "': "
                 << e.what() << endl;
            exit( 31 );
          }//try / catch
        }//if( summ_meas_for_single_out && (info.num_measurements() > 1) )
        
        
        files_to_combine.push_back( info );
      }else
      {
        const pair<bool,bool> wrote_out = write_output_file( info, format, saveto, inname );
        const bool wrote_all_out = wrote_out.first;
        //const bool a_file_already_existed = wrote_out.second;
        wrote_all = (wrote_all && wrote_all_out);
      }//if( combine_all_files ) / else
    }catch( std::exception &e )
    {
      cerr << "Error with input file '" << inputfiles[i] << "': " << e.what() << endl;
    }
  }//for( size_t i = 0; i < inputfiles.size(); ++i )
  
  if( combine_all_files )
  {
    if( files_to_combine.empty() )
    {
      cerr << "No files are available to combine." << endl;
      return 27;
    }//if( files_to_combine.empty() )
    
    if( files_to_combine.size() < 2 )
    {
      cerr << "Only one file was read in - not creating output since there is nothing to combine."
      << endl;
      return 28;
    }//if( files_to_combine.empty() )
    
    // We will just take model number and all that from the first measurement.
    //  TODO: improve this, or add warnings to the user?
    SpecUtils::SpecFile info = files_to_combine.front();
    
    for( size_t i = 1; i < files_to_combine.size(); ++i )
    {
      const SpecUtils::SpecFile &other = files_to_combine[i];
      
      for( const shared_ptr<const SpecUtils::Measurement> &m : other.measurements() )
      {
        auto nm = make_shared<SpecUtils::Measurement>( *m );
        info.add_measurement( nm, false );
      }
      
      bool added_remarks = false, added_warning = false;
      vector<string> updated_remarks, updated_warnings;
      const vector<string> &info_remarks = info.remarks();
      const vector<string> &other_remarks = other.remarks();
      for( const string &rem : other_remarks )
      {
        if( std::find(begin(info_remarks), end(info_remarks), rem) == end(info_remarks) )
        {
          if( updated_remarks.empty() )
            updated_remarks = info_remarks;
          updated_remarks.push_back( rem );
        }//if( std::find(begin(info_remarks), end(info_remarks), rem) == end(info_remarks) )
      }//for( const string &rem : other_remarks )
      
      const vector<string> &info_warnings = info.parse_warnings();
      const vector<string> &other_warnings = other.parse_warnings();
      for( const string &warn : other_warnings )
      {
        if( std::find(begin(info_warnings), end(info_warnings), warn) == end(info_warnings) )
        {
          if( updated_warnings.empty() )
            updated_warnings = info_warnings;
          updated_warnings.push_back( warn );
        }//if( std::find(begin(info_remarks), end(info_remarks), rem) == end(info_remarks) )
      }//for( const string &rem : other_remarks )
      
      
      if( !updated_remarks.empty() )
        info.set_remarks( updated_remarks );
      
      if( !updated_warnings.empty() )
        info.set_parse_warnings( updated_warnings );
    }//for( size size_t i = 1; i < files_to_combine.size(); ++i )
    
    
    unsigned int cleanup_flags = SpecUtils::SpecFile::CleanupAfterLoadFlags::StandardCleanup;
    if( combine_files_sort.empty() )
    {
      cleanup_flags = SpecUtils::SpecFile::CleanupAfterLoadFlags::DontChangeOrReorderSamples;
    }else if( combine_files_sort == "time" )
    {
      cleanup_flags = SpecUtils::SpecFile::CleanupAfterLoadFlags::ReorderSamplesByTime;
    //}else if( (combine_files_sort == "count-rate-increasing")
    //         || (combine_files_sort == "count-rate-decreasing") )
    //{
    }else
    {
      assert( 0 );
    }
    
    try
    {
      info.set_uuid( "" );
      info.cleanup_after_load( cleanup_flags );
    }catch( std::exception &e )
    {
      cerr << "Error organizing combined file contents: " << e.what() << endl;
      return 29;
    }
    
    if( sum_all_spectra )
    {
      shared_ptr<SpecUtils::Measurement> summed_meas = info.sum_measurements( info.sample_numbers(),
                                                                  info.detector_names(), nullptr );
      for( shared_ptr<const SpecUtils::Measurement> &m : info.measurements() )
        info.remove_measurement( m, false );
      info.add_measurement( summed_meas, true );
      try
      {
        info.set_uuid( "" );
        info.cleanup_after_load();
      }catch( std::exception &e )
      {
        cerr << "Error summing all spectra from summed files: "
             << e.what() << endl;
        return 30;
      }//try / catch
    }//if( sum_all_spectra )
    
    
    const string inname = "combination";
    string saveto = outputname;
    
    if( !SpecUtils::iequals_ascii( SpecUtils::file_extension(saveto), "."+ending ) )
      saveto += "." + ending;
    
    const pair<bool,bool> wrote_out = write_output_file( info, format, saveto, inname );
    const bool wrote_all_out = wrote_out.first;
    //const bool a_file_already_existed = wrote_out.second;
    wrote_all = (wrote_all && wrote_all_out);
  }//if( combine_all_files )
  
  
  if( file_existed )
    return 5;
  
  if( input_didnt_exist )
    return 6;
  
  if( !parsed_all )
    return 7;
  
  if( !wrote_all )
    return 8;
  
  return 0;
}//int run_command_util( int argc, char *argv[] )



bool requested_command_line_from_gui( const int argc, char *argv[] )
{
  for( int i = 0; i < argc; ++i )
  {
    string arg = argv[i];
    SpecUtils::trim( arg );
    if( SpecUtils::iequals_ascii( arg, "--convert" ) )
    {
      return true;
    }
    
    if( arg.size() > 1 && arg[0]=='-' && isalpha(arg[1])
        && SpecUtils::contains(arg, "c")
        && !SpecUtils::contains(arg, "-NS") ) //-NSDocumentRevisionsDebugMode
    {
      return true;
    }
  }//for( int i = 0; i < argc; ++i )
  
  return false;
}//bool requested_command_line_from_gui( int argc, char *argv[] )

}//namespace CommandLineUtil
