#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

using namespace std;
using boost::filesystem::path;
namespace po = boost::program_options;

int main( int argc, char **argv )
{
  po::options_description desc("Allowed options");
  
  desc.add_options()
  ( "help,h", "produce help message");
  
  po::variables_map vm;
  
  try
  {
    po::store( po::parse_command_line( argc, argv, desc ), vm );
    po::notify( vm );
  }catch( std::exception &e )
  {
    cerr << "Invalid command line argument\n\t" << e.what() << endl;
    return EXIT_FAILURE;
  }
  
  if( vm.count( "help" ) )
  {
    cout << desc << "\n";
    return EXIT_SUCCESS;
  }

  return EXIT_SUCCESS;
}//int main( int argc, char **argv )


