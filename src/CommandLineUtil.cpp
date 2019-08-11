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
#include <fstream>
#include <iostream>

#include <boost/program_options.hpp>

#include "SpecUtils_config.h"
#include "cambio/CommandLineUtil.h"
#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"

using namespace std;
namespace po = boost::program_options;

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
    std::ifstream t( filename );
    
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
  void add_file_meta_info_to_json( std::ostream &output, const MeasurementInfo &info )
  {
    auto jsonEscape = []( string input ) -> string {
      UtilityFunctions::ireplace_all( input, "\"", "\\\"" );  //ToDo: Properly escape JSON strings!
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
    
    if( !info.measurment_operator().empty() )
      output << ", Operator: \"" << jsonEscape( info.measurment_operator() ) << "\"";
    
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
    //const std::vector<std::string> &MeasurementInfo::detector_names() const
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
  }//void add_file_meta_info_to_json( std::ostream &ostr, const MeasurementInfo &info )
#endif
}
#endif

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
  bool sum_all_spectra;
  
  bool no_background_spec, no_foreground_spec, no_intrinsic_spec;
  bool no_calibration_spec, no_unknown_spec;
  bool background_only, foreground_only, calibation_only, intrinsic_only;
  //bool spectra_of_likely_interest_only;
  
#if( SpecUtils_ENABLE_D3_CHART )
  string html_to_include = "all";
#endif
  unsigned int rebin_factor;
  
  string outputname, outputformatstr;
  vector<string> inputfiles;
  
  string newserialnum, newdettype;
  
  po::options_description cl_desc("Allowed options");
  cl_desc.add_options()
    ("help,h",  "produce this help message")
    ("about,a",  "produce the about message")
    ("input,i", po::value< vector<string> >(&inputfiles),
              "input spectrum file(s)")
    ("output,o", po::value<string>(&outputname),
              "Output file or directory; if multiple input files are specified,"
              " this must be a valid existing directory.")
    ("format,f", po::value<string>(&outputformatstr),
              "Format of output spectrum file.  Must be specified when there"
              " are multiple input files, or if the output name for a single"
              " spectrum file is ambigous.  For a single file, if this option"
              " is not specified, the output names file extention will be used"
              " to guess output format desired.\n"
              "Possible values are: \tTXT, CSV, PCF, XML (2006 N42 format),"
              " N42 (defaults to 2012 variant), 2012N42, 2006N42,"
              " CHN (binary integer variant), SPC (defaults to int variant),"
              " INTSPC, FLTSPC, SPE (IAEA format), asciispc (ASCII version of"
              " SPC), gr130 (256 channel binary format)"
#if( SpecUtils_ENABLE_D3_CHART )
              ", html (webpage plot), json (chart data in json format, equiv to '--format=html --html-output=json')"
#endif
     )
  
    ("force", po::value<bool>(&force_writing)->default_value(false),
              "Forces overwriting of output file."
    )
    ("combine-multi", po::value<bool>(&summ_meas_for_single_out)->default_value(false),
              "For input files with multiple spectra, being saved to a output"
              " format that only allows a single spectrum, this option"
              " specifies to sum all spectra in the input file and save it"
              " as a single spectrum in the single ouput file.  By default a"
              " seperate output file will be created for each spectrum in the"
              " input file.  This option does not effect saving input files"
              " with many spectra to a format that supports mutliple spectra ("
              "which creates a single ouput file with multiple spectra).")
    ("all-calibrations", po::value<bool>(&include_all_cal_spec)->default_value(false),
              "Some input spectra will include the same gamma spectra more than"
              " once, but with different energy binning or calibration. By"
              " default Cambio will pick the calibration that makes the most"
              " sense (e.x., linear over compressed, or larger energy range"
              " over smaller, etc.) only include that in the output.  If you"
              " set this flag to true then all calibrations will be used to"
              " write the output."
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
    ("no-background-spec", po::value<bool>(&no_background_spec)->default_value(false),
      "Removes all spectra explicitly marked, or inferred from file format, as backgrounds."
    )
    ("no-foreground-spec", po::value<bool>(&no_foreground_spec)->default_value(false),
     "Removes all spectra explicitly marked, or inferred from file format, as foreground."
     " Input files with a single spectrum will be assumed as foreground, unless"
     " explicitly marked otherwise within the file."
    )
    ("no-intrinsic-spec", po::value<bool>(&no_intrinsic_spec)->default_value(false),
     "Removes all spectra explicitly marked, or inferred from file format, as"
     " the detectors intrinsic radiation spectrum set at the factory."
    )
    ("no-calibration-spec", po::value<bool>(&no_calibration_spec)->default_value(false),
     "Removes all spectra explicitly marked, or inferred from file format, as"
     " calibration spectra."
    )
    ("no-unknown-spec", po::value<bool>(&no_unknown_spec)->default_value(false),
     "Removes all spectra that do not have their type explcitly marked or"
     " designated by the file format.  For files that have a single spectrum,"
     " unless it is explicitly marked, it will be assumed a foreground."
    )
    ("background-only", po::value<bool>(&background_only)->default_value(false),
     "Only allow spectra marked as background to be written to the output."
    )
    ("foreground-only", po::value<bool>(&foreground_only)->default_value(false),
     "Filter out all none-foreground spectra."
     " Input files with a single unmarked sample will be assumed as foreground."
    )
    ("calibation-only", po::value<bool>(&calibation_only)->default_value(false),
     "Filter out all spectra not marked as calibration."
    )
    ("intrinsic-only", po::value<bool>(&intrinsic_only)->default_value(false),
     "Filter out all spectra not marked as instrinsic."
    )
    ( "sum-all-spectra", po::value<bool>(&sum_all_spectra)->default_value(false),
       "Sum all spectra in each of the input files, which pass any optional"
       " filters, so that the output files contains only a single spectrum, even"
       " if the output format could support more than a single spectrum."
    )
     ( "rebin-factor", po::value<unsigned int>(&rebin_factor)->default_value(1),
       "How many times to double the binning to reduce final number of bins."
       " 1 is no change, 2 is half as many bins as original, 3 is one fourth,"
       " 4 is one eight, and so on.  If it is asked to combine more channels than"
       " can be (ex., for 1024 channels, a rebin factor > 10), then it will be rebined"
       " down to a single channel."
     )
#if( SpecUtils_ENABLE_D3_CHART )
  ("html-output", po::value<string>(&html_to_include)->default_value("all"),
     "Only applies when saving to the HTML format.  The components to include"
     " in the output file.  Options are \n\t"
       "'all': includes everything making a self contained html file \n\t"
       "'json': only the data compnents of the spectra\n\t"
       "'css': the default styling of the charts\n\t"
       "'js': the SpectrumChartD3 library\n\t"
       "'d3': the D3 javascript library\n\t"
       "'controls': the html and js for display options")
#endif
  ;
  
  const char *about_msg =
"                            Cambio, Command Line Tool                          \n"
"                                      v2.1                                     \n"
"                                                                               \n"
"Please email wcjohns@sandia.gov AND cambio@sandia.gov with bug reports,        \n"
"suggestions, requests, or new file formats.                                    \n"
"Please visit https://hekili.ca.sandia.gov/CAMBIO/ for updates and additional   \n"
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
    cout << endl << "Example uses:\n"
         << "\t" << argv[0] << " input.n42 output.pcf" << endl
         << "\t" << argv[0] << " --format=CSV --no-background-spec=true ./path/to/input/*.n42 ./path/to/output" << endl
         << "\t" << argv[0] << " --format=n42 --sum-all-spectra=1 --input input_0.pcf input_1.csv input_3.spe --output ./output/" << endl
         << "\t" << argv[0] << " --sum-all-spectra true -o output.spc -i inputfile.n42" << endl
#if( SpecUtils_ENABLE_D3_CHART )
         << "\t" << argv[0] << " input.n24 output.html"
#endif
         << endl;
  };
  
  po::positional_options_description p;
  p.add("input", -1);
  
  
  po::variables_map cl_vm;

  
//  instance of basic_option<charT> will be added to result,
//  with 'unrecognized' field set to 'true'. It's possible to
//  collect all unrecognized options with the 'collect_unrecognized'
//  funciton.
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
  
  if( cl_vm.count("help") )
  {
    cout << cl_desc << endl;
    printExampleMsg();
    return 0;
  }//if( cl_vm.count("help") )

  
  //Map save to extension to save to types
  map<string,SaveSpectrumAsType> str_to_save_type;
  str_to_save_type["txt"]     = kTxtSpectrumFile;
  str_to_save_type["csv"]     = kCsvSpectrumFile;
  str_to_save_type["pcf"]     = kPcfSpectrumFile;
  str_to_save_type["xml"]     = kXmlSpectrumFile;
  str_to_save_type["n42"]     = k2012N42SpectrumFile;
  str_to_save_type["2012n42"] = k2012N42SpectrumFile;
  str_to_save_type["2006n42"] = kXmlSpectrumFile;
  str_to_save_type["chn"]     = kChnSpectrumFile;
  str_to_save_type["spc"]     = kBinaryIntSpcSpectrumFile;
  str_to_save_type["intspc"]  = kBinaryIntSpcSpectrumFile;
  str_to_save_type["fltspc"]  = kBinaryFloatSpcSpectrumFile;
  
  str_to_save_type["asciispc"]  = kAsciiSpcSpectrumFile;
  str_to_save_type["gr130"]     = kExploraniumGr130v0SpectrumFile;
  str_to_save_type["gr135"]     = kExploraniumGr135v2SpectrumFile;
  str_to_save_type["dat"]       = kExploraniumGr135v2SpectrumFile;
  str_to_save_type["spe"]       = kIaeaSpeSpectrumFile;

#if( SpecUtils_ENABLE_D3_CHART )
  str_to_save_type["html"]       = kD3HtmlSpectrumFile;
  str_to_save_type["json"]       = kD3HtmlSpectrumFile;
  str_to_save_type["js"]         = kD3HtmlSpectrumFile;
  str_to_save_type["css"]        = kD3HtmlSpectrumFile;
#endif

  
  //spec_exts: extensions of files that we can read.
  const string spec_exts[] = { "txt", "csv", "pcf", "xml", "n42", "chn", "spc", "dat", "cnf", "spe", "js", "json", "html", "css" };
  const size_t len_spec_exts = sizeof(spec_exts)/sizeof(spec_exts[0]);

  
  if( inputfiles.empty() || inputfiles[0].empty() )
  {
    cerr << "No input files specified." << endl;
    
    cout << "\n" << about_msg << endl;
    cout << cl_desc << endl;
    printExampleMsg();
    
    return 2;
  }//if( inputfiles.empty() )
  
  
  if( outputname.empty() )
  {
    if( inputfiles.size() > 1 )
    {
      outputname = inputfiles.back();
      inputfiles.pop_back();
    }else
    {
      if( !str_to_save_type.count(outputformatstr) )
      {
        cerr << "No ouput file/directory specified" << endl;
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
  
  
  if( inputfiles.size() > 1 && !UtilityFunctions::is_directory(outputname) )
  {
    cerr << "You must specify an output directory when there are mutliple input"
         << " files" << endl;
    return 3;
  }//if( mutliple input files, and not a output directory )
  
  
  if( !force_writing && UtilityFunctions::is_file(outputname) )
  {
    cerr << "Output file ('" << outputname << "') already exists; you can force"
         << " overwriting it by using the --force option." << endl;
    return 5;
  }//if( mutliple input files, and not a output directory )
  
  
  if( outputformatstr.empty() && inputfiles.size() > 1 )
  {
    cerr << "When mutliple input files are specified, you must also specify"
         << " the ouput format using the --format option" << endl;
    return 4;
  }//if( outputformatstr.empty() && inputfiles.size() > 1 )
  

  if( outputformatstr.empty() && outputname.size()>3 )
  {
    const string::size_type pos = outputname.find_last_of( '.' );
    if( (pos != string::npos) && (pos < (outputname.size()-1)) )
      outputformatstr = outputname.substr(pos+1);
  }//if( outputformatstr.empty() )
  
  UtilityFunctions::trim( outputformatstr );
  UtilityFunctions::to_lower( outputformatstr );
    
  if( str_to_save_type.find(outputformatstr) == str_to_save_type.end() )
  {
    if( outputformatstr.size() )
      cerr << "Output format type specified ('" << outputformatstr << "'), is"
           << " invalid.";
    else
      cerr << "Output format desired couldnt be guessed, use the --format flag"
           << " to specify.";
    
    cerr << "  Valid values are:\n\t";
    for( map<string,SaveSpectrumAsType>::const_iterator i = str_to_save_type.begin();
         i != str_to_save_type.end(); ++i )
      cerr << i->first << ", ";
    cerr << endl;
      
    return 4;
  }//if( user specified invalid type )
  
  SaveSpectrumAsType format = str_to_save_type[outputformatstr];
  
  
  assert( num_OutputMetaInfoDetectorNames == NumOutputMetaInfoDetectorType );
  OutputMetaInfoDetectorType metatype = NumOutputMetaInfoDetectorType;
  
  for( OutputMetaInfoDetectorType i = OutputMetaInfoDetectorType(0);
       i < NumOutputMetaInfoDetectorType; i = OutputMetaInfoDetectorType(i+1) )
  {
    if( UtilityFunctions::iequals( newdettype, OutputMetaInfoDetectorNames[i] ) )
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
         format = kBinaryIntSpcSpectrumFile;
    break;
      
    case GR130:
      if( outputformatstr == "dat" )
        format = kExploraniumGr130v0SpectrumFile;
    break;
      
    case GR135:
      if( outputformatstr == "dat" )
        format = kExploraniumGr135v2SpectrumFile;
      break;
      
    case identiFINDER1:
    case identiFINDERNG:
    case identiFINDERLaBr3:
      if( outputformatstr == "spc" )
        format = kAsciiSpcSpectrumFile;
      break;
      
    case NumOutputMetaInfoDetectorType:
      break;
  }//switch( metatype )
  
  
  //Make sure all the input files exist
  for( size_t i = 0; i < inputfiles.size(); ++i )
  {
    if( !UtilityFunctions::is_file(inputfiles[i]) )
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
  if( calibation_only )
    no_background_spec = no_foreground_spec = no_intrinsic_spec = no_unknown_spec = true;
  if( intrinsic_only )
    no_background_spec = no_foreground_spec = no_calibration_spec = no_unknown_spec = true;

  
  string outdir, outname;
  if( UtilityFunctions::is_directory(outputname) )
  {
    outname = "";
    outdir = outputname;
  }else
  {
    outname = UtilityFunctions::filename(outputname);
    outdir = UtilityFunctions::parent_path(outputname);
  }
  
  string ending = suggestedNameEnding( format );
  
 #if( SpecUtils_ENABLE_D3_CHART )
  if( format == kD3HtmlSpectrumFile )
  {
    UtilityFunctions::to_lower( html_to_include );
    
    if( outputformatstr == "json" )
      html_to_include = "json";
    
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
  
  bool parsed_all = true, input_didnt_exist = false,
       file_existed = false, wrote_all = true;
  
  for( size_t i = 0; i < inputfiles.size(); ++i )
  {
    try
    {
      if( !UtilityFunctions::is_file(inputfiles[i]) )
      {
        input_didnt_exist = true;
        cerr << "Input file '" << inputfiles[i] << "' doesnt exist, or cant be"
             << " accessed." << endl;
        continue;
      }//if( input file didnt exist )
    
      MeasurementInfo info;
    
      const string inname = inputfiles[i];
      const bool loaded = info.load_file( inname, kAutoParser, inname );
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
        
        string prefered_variant;
        for( const auto str : cals )
        {
          if( UtilityFunctions::icontains( str, "Lin") )
            prefered_variant = str;
        }//for( const auto &str : cals )
        
        if( prefered_variant.empty() )
        {
          auto getMev = []( string str ) -> double {
            UtilityFunctions::to_lower(str);
            size_t pos = str.find("mev");
            if( pos == string::npos )
              return -999.9;
            str = str.substr(0,pos);
            UtilityFunctions::trim( str );
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
          info.keep_energy_cal_variant( prefered_variant );
        }else
        {
          cerr << "Couldnt identify a prefered energy variant out of {";
          int calnum = 0;
          for( const auto str : cals )
            cerr << (calnum++ ? ", " : "") << str;
          cerr << "} - so including all energy calibrations" << endl;
        }
      }//if( more than one calibration present, and we only want one )
      
      const set<int> prefilter_samples = info.sample_numbers();
      
      auto remove_type = [&info,inname,prefilter_samples]( Measurement::SourceType type ){
        int nremoved = 0;
        vector<shared_ptr<const Measurement>> meass = info.measurements();
        for( shared_ptr<const Measurement> &m : meass )
        {
          Measurement::SourceType record_type = m->source_type();
          
          if( record_type == Measurement::SourceType::UnknownSourceType
             && prefilter_samples.size()==1 )
            record_type = Measurement::SourceType::Foreground;
          
          if( record_type == type )
          {
            ++nremoved;
            info.remove_measurment( m, false );
          }
        }//for( loop over measurements )
        
        if( nremoved )
        {
          try
          {
            info.cleanup_after_load();
          }catch( std::exception &e )
          {
            cerr << "Error removing spectra from '" << inname << "': " << e.what() << endl;
          }//try / catch
        }//if( nremoved )
      };//remove_type(...)
      
      if( no_background_spec )
        remove_type( Measurement::SourceType::Background );
      
      if( no_foreground_spec )
        remove_type( Measurement::SourceType::Foreground );
      
      if( no_intrinsic_spec )
        remove_type( Measurement::SourceType::IntrinsicActivity );
      
      if( no_calibration_spec )
        remove_type( Measurement::SourceType::Calibration );
      
      if( no_unknown_spec )
        remove_type( Measurement::SourceType::UnknownSourceType );
      
      if( sum_all_spectra )
      {
        const set<int> sample_num = info.sample_numbers();
        const std::vector<int> det_nums = info.detector_numbers();
        const vector<bool> det_to_use( det_nums.size(), true );
        std::shared_ptr<Measurement> summed_meas = info.sum_measurements( sample_num, det_to_use);
        vector<shared_ptr<const Measurement>> meass = info.measurements();
        for( shared_ptr<const Measurement> &m : meass )
          info.remove_measurment( m, false );
        info.add_measurment( summed_meas, true );
        try
        {
          info.cleanup_after_load();
        }catch( std::exception &e )
        {
          cerr << "Error summing all spectra from '" << inputfiles[i] << "': "
               << e.what() << "\n\tSkipping file." << endl;
          continue;
        }//try / catch
      }//if( sum_all_spectra )
      
      if( rebin_factor > 1 )
      {
        set<size_t> nchannels;
        for( const auto &m : info.measurements() )
          nchannels.insert( m->num_gamma_channels() );
        
        for( const size_t nchann : nchannels )
        {
          if( nchann < 2 )
            continue;
          
          const size_t ncombine = std::pow( 2, (rebin_factor-1) );
          if( (nchann % ncombine) != 0 )
          {
            cerr << "Not rebinning spectra with " << nchann << " as " << nchann
                 << "%" << ncombine << "=" << (nchann % ncombine) << endl;
            continue;
          }
          
          info.combine_gamma_channels( ncombine, nchann );
        }//for( const size_t nchann : nchannels )
      }//if( rebin_factor > 1 )
      
      
      string savename = outname;
      if( savename.empty() )
      {
        savename = UtilityFunctions::filename(inname);
        const string::size_type pos = savename.find_last_of( '.' );
        if( pos && (pos != string::npos) && (pos < (savename.size()-1)) )
        {
          string ext = savename.substr(pos+1);
          UtilityFunctions::to_lower( ext );
        
          if( std::count(spec_exts, spec_exts+len_spec_exts, ext) )
            savename = savename.substr( 0, pos );
        }//if( pos != string::npos )
      }//if( savename.empty() )
    
      const string::size_type pos = savename.find_last_of( '.' );
      if( pos && (pos != string::npos) && (pos < (savename.size()-1)) )
      {
        string ext = savename.substr(pos+1);
        UtilityFunctions::to_lower( ext );
        if( ext != ending )
          savename += "." + ending;
      }else
        savename += "." + ending;
    
      const string saveto = UtilityFunctions::append_path( outdir, savename );
    
    
      if( saveto == inname )
      {
        cerr << "Output file '" << saveto << "' identical to input file name,"
             << " not saving file" << endl;
        file_existed = true;
        continue;
      }
    
      vector< std::shared_ptr<const Measurement> > meass = info.measurements();
      
      
      
      bool containted_netruon = false;
      for( size_t i = 0; i < meass.size(); ++i )
        containted_netruon |= meass[i]->contained_neutron();
      
      switch( metatype )
      {
        case DetectiveEX:
          for( size_t i = 0; i < meass.size(); ++i )
            if( !meass[i]->contained_neutron() )
              info.set_contained_neutrons( true, 0.0, meass[i] );

          if( info.detector_type() == kDetectiveExDetector )
            break;
          
          info.set_instrument_id( "240 Detective-EX" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveEX" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kDetectiveExDetector );
        break;
          
        case DetectiveDX:
          for( size_t i = 0; i < meass.size(); ++i )
            info.set_contained_neutrons( false, 0.0f, meass[i] );
          
          if( info.detector_type() == kDetectiveExDetector )
            break;
          
          info.set_instrument_id( "240 Detective-DX" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveDX" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kDetectiveExDetector );
        break;
          
        case uDetective:
          if( info.detector_type() == kMicroDetectiveDetector )
            break;
          
          info.set_instrument_id( "7258 MicroDetective" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "MicroDetective" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kDetectiveExDetector );
        break;
          
        case DetectiveEX100:
          for( size_t i = 0; i < meass.size(); ++i )
            if( !meass[i]->contained_neutron() )
              info.set_contained_neutrons( true, 0.0, meass[i] );

          if( info.detector_type() == kDetectiveEx100Detector )
            break;
          
          info.set_instrument_id( "7000 Detective-EX100" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveEX100" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kDetectiveEx100Detector );
          break;
          
        case DetectiveDX100:
          for( size_t i = 0; i < meass.size(); ++i )
            info.set_contained_neutrons( false, 0.0f, meass[i] );
          
          if( info.detector_type() == kDetectiveEx100Detector )
            break;
          
          info.set_instrument_id( "7000 Detective-DX100" );
          info.set_manufacturer( "ORTEC" );
          info.set_instrument_model( "DetectiveDX100" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kDetectiveEx100Detector );
        break;
          
        case GR130:
          break;
          
        case GR135:
          break;
          
        case identiFINDER1:
          info.set_manufacturer( "FLIR" );
          info.set_instrument_model( "identiFINDER" );
          info.set_detector_type( kIdentiFinderDetector );
          info.set_instrument_type( "RadionuclideIdentifier" );
          break;
          
        case identiFINDERNG:
          if( containted_netruon )
            info.set_instrument_model( "identiFINDER 2 NGH" );
          else
            info.set_instrument_model( "identiFINDER 2 NG" );
          info.set_manufacturer( "FLIR" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kIdentiFinderNGDetector );
          break;
          
        case identiFINDERLaBr3:
          if( containted_netruon )
            info.set_instrument_model( "identiFINDER 2 LGH" );
          else
            info.set_instrument_model( "identiFINDER 2 LG" );
          info.set_manufacturer( "FLIR" );
          info.set_instrument_type( "RadionuclideIdentifier" );
          info.set_detector_type( kIdentiFinderLaBr3Detector );
          break;
          
        case NumOutputMetaInfoDetectorType:
          break;
      }//switch( metatype )
      
      
      if( cl_vm.count("set-serial-number") )
        info.set_instrument_id( newserialnum );
    
      bool opened_all_output_files = true, encoded_all_files = true;
    
      if( info.measurements().empty() )
      {
        //encoded_all_files = false;
        cerr << "After filtering, '" << inname << "' had no spectra left to write - skipping." << endl;
        continue;
      }//if( we filteres out all the measuremnts )
      
      
      if( format == kChnSpectrumFile
          || format == kBinaryIntSpcSpectrumFile
          || format == kBinaryFloatSpcSpectrumFile
          || format == kAsciiSpcSpectrumFile
          || format == kIaeaSpeSpectrumFile
         )
      {
        const vector< MeasurementConstShrdPtr > meass = info.measurements();
        const std::set<int> samplenums = info.sample_numbers();
        const std::vector<int> detnums = info.detector_numbers();
      
        if( meass.size()<2 || summ_meas_for_single_out )
        {
          if( !force_writing && UtilityFunctions::is_file(saveto) )
          {
            cerr << "Output file '" << saveto << "' existed, and --force not"
                 << " specified, not saving file" << endl;
            file_existed = true;
            continue;
          }//if( !force_writing && UtilityFunctions::is_file(savename) )
    
        
          ofstream output( saveto.c_str(), ios::binary | ios::out );
        
          if( !output.is_open() )
          {
            cerr << "Failed to open output file " << saveto << endl;
            opened_all_output_files = false;
            continue;
          }
        
          bool wrote;
          std::set<int> detnumset;
          for( size_t i = 0; i < detnums.size(); ++i )
            detnumset.insert( detnums[i] );
        
          if( format == kChnSpectrumFile )
            wrote = info.write_integer_chn( output, samplenums, detnumset );
          else if( format == kBinaryIntSpcSpectrumFile )
            wrote = info.write_binary_spc( output, MeasurementInfo::IntegerSpcType, samplenums, detnumset );
          else if( format == kBinaryFloatSpcSpectrumFile )
            wrote = info.write_binary_spc( output, MeasurementInfo::FloatSpcType, samplenums, detnumset );
          else if( format == kAsciiSpcSpectrumFile )
            wrote = info.write_ascii_spc( output, samplenums, detnumset );
          else if( format == kIaeaSpeSpectrumFile )
            wrote = info.write_iaea_spe( output, samplenums, detnumset );
          else
            assert( 0 );
        
          if( !wrote )
          {
            encoded_all_files = false;
            cerr << "Possibly failed in writing '" << saveto << "'" << endl;
          }else
          {
            cout << "Saved '" << saveto << "'" << endl;
          }
        }else  //if( sum all measurments )
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
                int nchar = strlen(buffer);
                while( nchar++ < 4 )  //VS2012 doesnt support %4d format flag
                  outname += "0";
                outname += buffer;
              }
              
              if( extention.size() > 0 )
                outname = outname + extention;
            
              if( !force_writing && UtilityFunctions::is_file(outname) )
              {
                cerr << "Output file '" << outname << "' existed, and --force not"
                     << " specified, not saving file" << endl;
                file_existed = true;
                continue;
              }//if( !force_writing && UtilityFunctions::is_file(savename) )

            
              ofstream output( outname.c_str(), ios::binary | ios::out );
            
              if( !output.is_open() )
              {
                cerr << "Failed to open output file " << outname << endl;
                opened_all_output_files = false;
                continue;
              }else
              {
                bool wrote;
                if( format == kChnSpectrumFile )
                  wrote = info.write_integer_chn( output, samplenumset, detnumset );
                else if( format == kBinaryIntSpcSpectrumFile )
                  wrote = info.write_binary_spc( output, MeasurementInfo::IntegerSpcType, samplenumset, detnumset );
                else if( format == kBinaryFloatSpcSpectrumFile )
                  wrote = info.write_binary_spc( output, MeasurementInfo::FloatSpcType, samplenumset, detnumset );
                else if( format == kAsciiSpcSpectrumFile )
                  wrote = info.write_ascii_spc( output, samplenums, detnumset );
                else if( format == kIaeaSpeSpectrumFile )
                  wrote = info.write_iaea_spe( output, samplenums, detnumset );
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
      }else  //if( a single spectrum output format )
      {
        if( !force_writing && UtilityFunctions::is_file(saveto) )
        {
          cerr << "Output file '" << saveto << "' existed, and --force not"
               << " specified, not saving file" << endl;
          file_existed = true;
          continue;
        }//if( !force_writing && UtilityFunctions::is_file(savename) )
      
        ofstream output( saveto.c_str(), ios::binary | ios::out );
      
        if( !output.is_open() )
        {
          cerr << "Failed to open output file " << saveto << endl;
          opened_all_output_files = false;
          continue;
        }
      
        bool wrote = false;
        switch( format )
        {
          case kTxtSpectrumFile:
            wrote = info.write_txt( output );
          break;
          
          case kCsvSpectrumFile:
            wrote = info.write_csv( output );
          break;
          
          case kPcfSpectrumFile:
            wrote = info.write_pcf( output );
          break;
          
          case kXmlSpectrumFile:
            wrote = info.write_2006_N42( output );
          break;
          
          case k2012N42SpectrumFile:
            wrote = info.write_2012_N42( output );
          break;
          
          case kExploraniumGr130v0SpectrumFile:
            wrote = info.write_binary_exploranium_gr130v0( output );
          break;
            
          case kExploraniumGr135v2SpectrumFile:
            wrote = info.write_binary_exploranium_gr135v2( output );
          break;
            
#if( SpecUtils_ENABLE_D3_CHART )
          case kD3HtmlSpectrumFile:
          {
            const vector<std::shared_ptr<const Measurement> > measurements = info.measurements();
            
            //Should probably look to see if this is an obvious case where we
            //  should show the foreground/background on the same chart.
            if( UtilityFunctions::iequals( html_to_include, "json" ) )
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
            }else if( UtilityFunctions::iequals( html_to_include, "css" ) )
            {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
              output << D3SpectrumExport::spectrum_char_d3_css() << endl;
#else
              const char *incssname = D3SpectrumExport::spectrum_char_d3_css_filename();
              ifstream incss( incssname, ios::in | ios::binary );
              if( !incss.is_open() )
              {
                cerr << "Could not open the input CSS file '" << incssname << "'" << endl;
                return 11;
              }
              output << incss.rdbuf();
#endif
              break;
              wrote = output.good();
            }else if( UtilityFunctions::iequals( html_to_include, "js" ) )
            {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
              output << D3SpectrumExport::spectrum_chart_d3_js() << endl;
#else
              const char *incssname = D3SpectrumExport::spectrum_chart_d3_js_filename();
              ifstream incss( incssname, ios::in | ios::binary );
              if( !incss.is_open() )
              {
                cerr << "Could not open the input SpectrumChartD3 file '" << incssname << "'" << endl;
                return 11;
              }
              output << incss.rdbuf();
#endif
              wrote = output.good();
              break;
            }else if( UtilityFunctions::iequals( html_to_include, "d3" ) )
            {
#if( SpecUtils_D3_SUPPORT_FILE_STATIC )
              output << D3SpectrumExport::d3_js() << endl;
#else
              const char *incssname = D3SpectrumExport::d3_js_filename();
              ifstream incss( incssname, ios::in | ios::binary );
              if( !incss.is_open() )
              {
                cerr << "Could not open the input D3.js file '" << incssname << "'" << endl;
                return 11;
              }
              output << incss.rdbuf();
#endif
              wrote = output.good();
              break;
            }else if( UtilityFunctions::iequals( html_to_include, "controls" ) )
            {
              cerr << "Writing controls HTML is not supported yet - sorry" << endl;
              return 12;
            }else if( UtilityFunctions::iequals( html_to_include, "all" ) )
            {
              using namespace D3SpectrumExport;
              D3SpectrumChartOptions fileopts;
              D3SpectrumExport::D3SpectrumOptions specopts;
              specopts.line_color = "black";
              specopts.display_scale_factor = 1.0;
              
              fileopts.m_title = UtilityFunctions::filename( inname );
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
                std::shared_ptr<const Measurement> m = measurements[i];
                const string div_id = "chart" + std::to_string(i);
                
                fileopts.m_dataTitle = m->title();
                if( fileopts.m_dataTitle.empty() )
                {
                  if( m->detector_name() != "" )
                    fileopts.m_dataTitle = "Det " + m->detector_name() + " ";
                  fileopts.m_dataTitle += "Sample " + to_string(m->sample_number());
                }
              
                vector< pair<const Measurement *,D3SpectrumOptions> > htmlinput;
                htmlinput.push_back( std::pair<const Measurement *,D3SpectrumOptions>(m.get(),specopts) );
                
                output << "<div id=\"" << div_id << "\" class=\"chart\" oncontextmenu=\"return false;\";></div>" << endline;  // Adding the main chart div
                
                
                output << "<script>" << endline;
                write_js_for_chart( output, div_id, fileopts.m_dataTitle, fileopts.m_xAxisTitle, fileopts.m_yAxisTitle );
                write_and_set_data_for_chart( output, div_id, htmlinput );
                output << "window.addEventListener('resize',function(){spec_chart_" << div_id << ".handleResize();});" << endline;
                write_set_options_for_chart( output, div_id, fileopts );
                output << "</script>" << endline;
                
                write_html_display_options_for_chart( output, div_id, fileopts );
              }
              
              output << "</body>" << endline;
              output << "</html>" << endline;
              
              wrote = output.good();
            }else
            {
              cerr << "The 'html-output' option must specify exactly one of the"
                      " following: all, json, css, js, d3, controls."
                      "  You specified '" << html_to_include << "'" << endl;
              return 10;
            }
            break;
          }
#endif  //#if( SpecUtils_ENABLE_D3_CHART )
            
          case kChnSpectrumFile:
          case kBinaryIntSpcSpectrumFile:
          case kBinaryFloatSpcSpectrumFile:
          case kNumSaveSpectrumAsType:
          case kIaeaSpeSpectrumFile:
          case kAsciiSpcSpectrumFile:
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
      }//if( a single spectrum output format )
    }catch( std::exception &e )
    {
      
    }
  }//for( size_t i = 0; i < inputfiles.size(); ++i )
  
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
    UtilityFunctions::trim( arg );
    if( UtilityFunctions::iequals( arg, "--convert" ) )
    {
      return true;
    }
    
    if( arg.size() > 1 && arg[0]=='-' && isalpha(arg[1])
        && UtilityFunctions::contains(arg, "c")
        && !UtilityFunctions::contains(arg, "-NS") ) //-NSDocumentRevisionsDebugMode
    {
      return true;
    }
  }//for( int i = 0; i < argc; ++i )
  
  return false;
}//bool requested_command_line_from_gui( int argc, char *argv[] )

}//namespace CommandLineUtil
