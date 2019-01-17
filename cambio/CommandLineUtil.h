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

#ifndef CommandLineUtil_H
#define CommandLineUtil_H

namespace CommandLineUtil
{
  //run_command_util(): performs the command line proccessing and file
  //  conversion returning the following codes.
  //  0: successfully converted files
  //  1: invalid argument syntax
  //  2: no input file specified
  //  3: no output file/directory specified
  //  4: failed to specify (and couldn guess) output file type
  //  5: output file existed, and force overwrite not specified
  //  6: an input file didnt exist
  //  7: couldnt decode (some or all of the) input file(s)
  //  8: couldnt save (some or all of the) output file(s)
  //  9: invalid detector type specified to fill in meta-information
  //
  //Run the executable with the --help option to see command line argument
  //  options.
  int run_command_util( const int argc, char *argv[] );
  
  //requested_command_line_from_gui(): if the app is compiled to support both
  //  GUI and command line capabilities, then the user must add a --convert,
  //  or -c option, argument on the command line.
  bool requested_command_line_from_gui( const int argc, char *argv[] );
  
}//namespace CommandLineUtil

#endif //CommandLineUtil_H
