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


#ifndef CambioApp_H
#define CambioApp_H

#include <QString>
#include <QApplication>

#include "Cambio_config.h"
#include "cambio/MainWindow.h"

class QEvent;
class MainWindow;
class QCloseEvent;
class QSplashScreen;

#if( defined(ANDROID) )
#include <QRunnable>
//OpenFileRunable is a class used to post to the global QThreadPool and then
//  emit the signal for the CambioApp to open the specified file.  This is
//  all necassarry since CambioApp::loadFile() must be called from the main
//  GUI thread
class OpenFileRunable : public QObject, public QRunnable
{
  Q_OBJECT

public:
  OpenFileRunable( const QString &filepath );
  virtual ~OpenFileRunable();
  virtual void run();
  
signals:
  void doLoadFile(const QString &);

protected:
  const QString m_path;
};//class OpenFileRunable
#endif


class CambioApp : public QApplication
{
  Q_OBJECT
public:
	CambioApp( int &argc, char **argv );
  virtual ~CambioApp();
  
  MainWindow *mainWindow();
  
public slots:
  void loadFile( const QString &fileName );
  
#if( !defined(ANDROID) && !defined(IOS) && SHOW_CAMBIO_SPLASH_SCREEN )
  void createSplashScreen();
  void startSplashScreenFade();
  void deleteSplashScreen();
#endif
  
protected:
  bool event( QEvent *event );
  
  MainWindow *m_window;
  QSplashScreen *m_splash;
};//class CambioApp


#endif
