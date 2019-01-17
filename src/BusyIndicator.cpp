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

#include <assert.h>

#include <QWidget>

#include <QLabel>
#include <QFrame>
#include <QGridLayout>
#include <QApplication>

#include "cambio/BusyIndicator.h"



BusyIndicator::BusyIndicator( BusyType type, QWidget *p )
 : QWidget( p, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint ),
   m_type( type )
{
  setWindowModality( Qt::ApplicationModal );
  
  assert( p );
  
  resize( p->width()/2, p->height()/2 );
  move( p->width()/4, p->height()/4 );
  
  QFrame *frame = new QFrame( this );
  frame->resize( p->width()/2, p->height()/2 );
  
  setAttribute( Qt::WA_TranslucentBackground, true );
  setWindowOpacity( 0.5 );
  
  frame->setStyleSheet( "border-width: 2px;"
                        "border-style: solid;"
                        "border-radius: 15px;"
                        "border-color: rgba(225,225,225,125);"
                        "background: rgba(125,125,125,200);" );
  
  QGridLayout *layout = new QGridLayout();
  frame->setLayout( layout );
  
  
  QLabel *txt = 0;
  switch( m_type )
  {
    case OpeningFile:
      txt = new QLabel( "Parsing File", this );
    break;
      
    case SummingSpectra:
      txt = new QLabel( "Summing Spectra", this );
    break;
      
    case SavingSpectra:
      txt = new QLabel( "Saving Spectra", this );
    break;
  }//switch( m_type )
  
  txt->setStyleSheet( "border: none; background: none; color: white;" );
  QFont font;
  font.setBold( true );
  font.setPointSize( 32 );
  
  txt->setFont( font );
  
  layout->addWidget( txt, 0, 0, Qt::AlignHCenter | Qt::AlignVCenter );
}//BusyIndicator constructor


BusyIndicator::~BusyIndicator()
{
  
}
