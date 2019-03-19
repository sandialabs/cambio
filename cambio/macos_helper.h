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


#ifndef MACOS_HELPER_H
#define MACOS_HELPER_H

#include <QUrl>


/** For at least Qt 5.4.0, sometimes file URLS in macOS will look like:
   "file:///.file/id=9541167.50021164", so we need to call back obj-c to convert
   to a file URL like /Users/wcjohns/myspec.spc; this does this
 */
QUrl fromNSUrl( const QUrl &url );


#endif //MACOS_HELPER_H