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


#include <QRect>
#include <QDebug>
#include <QTimer>
#include <QFileInfo>
#include <QApplication>
#include <QSplashScreen>
#include <QDesktopWidget>
#include <QFileOpenEvent>
#include <QCoreApplication>
#include <QPropertyAnimation>

#include "cambio/CambioApp.h"
#include "cambio/sandia_logo.h"
#if( !defined(ANDROID) && !defined(IOS) && SHOW_CAMBIO_SPLASH_SCREEN )
#include "cambio/splash_screen.h"
#endif

#if( defined(ANDROID) )
#include <QAndroidJniObject>

OpenFileRunable::OpenFileRunable( const QString &path )
     : QRunnable(), m_path( path )
{
}

OpenFileRunable::~OpenFileRunable()
{
}

 void OpenFileRunable::run()
 {
   emit doLoadFile(m_path);
 }
#endif  //#if( defined(ANDROID) )


CambioApp::CambioApp( int &argc, char **argv )
  : QApplication( argc, argv ),
    m_window( new MainWindow() )
{
#if( !defined(ANDROID) && !defined(IOS) && SHOW_CAMBIO_SPLASH_SCREEN  )
  createSplashScreen();
#endif
  
  m_window->show();

  for( int i = 1; i < argc; ++i )
    loadFile( argv[i] );

#if( defined(ANDROID) )
  QAndroidJniObject::callStaticMethod<void>("org/sandia/cambio/CambioActivity", "openFileIfNeedBe" );
#endif
  
  //QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
}//CambioApp constructor


CambioApp::~CambioApp()
{
}


MainWindow *CambioApp::mainWindow()
{
  return m_window;
}


#if( !defined(ANDROID) && !defined(IOS) && SHOW_CAMBIO_SPLASH_SCREEN )
void CambioApp::createSplashScreen()
{
  if( m_splash )
    return;
  
  QPixmap pixmap;
  pixmap.loadFromData( splash_screen_png, splash_screen_png_len );  //182 kb
  
  m_splash = new QSplashScreen( pixmap );
  
  m_splash->show();
  
//  const char *msg = "";
//  m_splash->showMessage( msg );
  
  qApp->processEvents();
  
  QTimer::singleShot( 1000, this, SLOT(startSplashScreenFade()) );
}//void createSplashScreen()


void CambioApp::startSplashScreenFade()
{
  if( !m_splash )
    return;
  
  QPropertyAnimation *animation = new QPropertyAnimation( m_splash, "windowOpacity" );
  animation->setDuration( 500 );
  animation->setStartValue( 1.0 );
  animation->setEndValue( 0.0 );
  animation->start( QAbstractAnimation::DeleteWhenStopped );
  QObject::connect( animation, SIGNAL(finished()), this, SLOT(deleteSplashScreen()) );
}//void startSplashScreenFade()


void CambioApp::deleteSplashScreen()
{
  if( !m_splash )
    return;
  delete m_splash;
  m_splash = 0;
}//void deleteSplashScreen()
#endif //#if( !defined(ANDROID) && !defined(IOS) )


bool CambioApp::event( QEvent *event )
{
  switch( event->type() )
  {
    case QEvent::FileOpen:
    {
      QFileOpenEvent *e = dynamic_cast<QFileOpenEvent *>(event);
      if( e )
      {
        qDebug() << "Will try to open file";
        loadFile( e->file() );
        return true;
      }//if( e )
      
      break;
    }//case QEvent::FileOpen:
        
    default:
      break;
  }//switch( event->type() )
  
  return QApplication::event(event);
}//bool event( QEvent *event )
  

void CambioApp::loadFile( const QString &fileName )
{
  QFileInfo info( fileName );
  qDebug() << "CambioApp::loadFile: " << fileName << " info.isFile()" << info.isFile();
  if( info.isFile() || info.isDir() )
    m_window->handleSingleFileDrop( QUrl::fromLocalFile(fileName) );
}//void loadFile( const QString &fileName )

