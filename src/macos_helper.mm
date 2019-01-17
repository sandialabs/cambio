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


#include <Foundation/Foundation.h>
#include <QUrl>


#include "cambio/macos_helper.h"

QUrl fromNSUrl( const QUrl &url )
{
  NSURL *osxurl = url.toNSURL();
  NSString *nsstrpath = osxurl.path;
  
  //Its my *guess* that the pointers are garbage collected or something, but
  //  I really dont know objective-c
  
  QString qtstpath = QString::fromNSString( nsstrpath );
  return QUrl::fromLocalFile( qtstpath );
}//fromNSUrl(...)