//#include <jni.h>
#include <QDebug>
#include <QTimer>
#include <QRunnable>
#include <QThreadPool>
#include <QtAndroidExtras/QAndroidJniObject>

#include "cambio/CambioApp.h"


extern "C"
{
  JNIEXPORT
  jboolean
  JNICALL
  Java_org_sandia_cambio_CambioActivity_openFile
  ( JNIEnv* env, jobject /*thiz*/, jstring path )
  {
    jboolean loaded = JNI_FALSE;
    const char *str = env->GetStringUTFChars( path, 0 );

    const QString pathstr = str;
    CambioApp *app = dynamic_cast<CambioApp *>( QApplication::instance() );
    if( app && (pathstr.size() > 2) )
    {
      loaded = JNI_TRUE;
      //We have to jump through some hoops to make sure to call
      //  app->loadFile(...) from the main GUI thread
      OpenFileRunable *runner = new OpenFileRunable( pathstr );
      runner->setAutoDelete( true );
      QObject::connect( runner, SIGNAL(doLoadFile(const QString &)), app, SLOT(loadFile(const QString &)) );
      QThreadPool::globalInstance()->start( runner );
    }

    env->ReleaseStringUTFChars( path, str );

    return loaded;
  }//Java_eu_webtoolkit_android_WtAndroid_addopenfiletodb
}  //extern "C"
