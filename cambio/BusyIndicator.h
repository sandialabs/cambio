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


#ifndef BusyIndicator_H
#define BusyIndicator_H

#include <QWidget>

//When we open a file, or need to re-sum a large spectrum file, it can take
//  a while, so we'll put up an indicator.
//Right now it is intended you will create a BusyIndicator inside the main
//  GUI thread, and then do the long processing inside the GUI thread.  I had
//  experimented with doing the work outside the GUI thread, but the
//  communications between the threads was kinda a pain, and didnt work out
//  to well at first (probably due to my niavete of Qt threading system)
class BusyIndicator : public QWidget
{
  Q_OBJECT
  
public:
  enum BusyType
  {
    OpeningFile,
    SummingSpectra,
    SavingSpectra
  };//enum BusyType
  
  BusyIndicator( BusyType type, QWidget *parent );
  ~BusyIndicator();
  
protected:
  BusyType m_type;
};//class BusyIndicator


#endif
