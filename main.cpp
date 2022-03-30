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

  
#include "Cambio_config.h"

#if( BUILD_CAMBIO_COMMAND_LINE )
#include "cambio/CommandLineUtil.h"
#endif

#ifdef _WIN32
#include "SpecUtils/StringAlgo.h"
#endif

#if( BUILD_CAMBIO_GUI )
#include "cambio/CambioApp.h"

#if( defined(Q_OS_MAC) )
#include <QtPlugin>
Q_IMPORT_PLUGIN(QMacStylePlugin)
Q_IMPORT_PLUGIN(QCocoaIntegrationPlugin)
#endif

#if( defined(Q_OS_WIN) )
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin);
#endif
#endif


#if( !BUILD_CAMBIO_GUI && !BUILD_CAMBIO_COMMAND_LINE )
#error "You must have BUILD_CAMBIO_GUI and/or BUILD_CAMBIO_COMMAND_LINE enabled"
#endif

#ifdef _WIN32
void getUtf8Args( int &argc, wchar_t **wargv, char ** &argv );
#endif
#include <iostream>

#ifdef _WIN32
int wmain( int argc, wchar_t *wargv[] )
{
  char **argv;
  getUtf8Args( argc, wargv, argv );
#else
int main( int argc, char *argv[] )
{
#endif
    
#if( BUILD_CAMBIO_COMMAND_LINE )
#if( BUILD_CAMBIO_GUI )
  if( CommandLineUtil::requested_command_line_from_gui( argc, argv ) )
    return CommandLineUtil::run_command_util( argc, argv );
#else
  return CommandLineUtil::run_command_util( argc, argv );
#endif
#endif
  
  
#if( BUILD_CAMBIO_GUI )
  CambioApp a( argc, argv );
  
  return a.exec();
#endif
}//int main(int argc, char *argv[])


#ifdef _WIN32
#include "SpecUtils/StringAlgo.h"

/** Get command line arguments encoded as UTF-8.
 This function just leaks the memory
 */
void getUtf8Args( int &argc, wchar_t **argvw, char ** &argv )
{
  argv = (char **)malloc(sizeof(char *)*argc);
  
  for( int i = 0; i < argc; ++i)
  {
    //printf("Argument: %d: %ws\n", i, argvw[i]);
    const std::string asutf8 = SpecUtils::convert_from_utf16_to_utf8( argvw[i] );
    argv[i] = (char *)malloc( sizeof(char)*(asutf8.size()+1) );
    strcpy( argv[i], asutf8.c_str() );
  }//for( int i = 0; i < argc; ++i)
}//void processCustomArgs()

#endif //_WIN32
