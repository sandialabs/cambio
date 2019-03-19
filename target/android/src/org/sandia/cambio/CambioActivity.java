package org.sandia.cambio;

import org.qtproject.qt5.android.bindings.QtApplication;
import org.qtproject.qt5.android.bindings.QtActivity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.util.*;
import android.net.*;


public class CambioActivity extends QtActivity
{
  private static CambioActivity m_instance;
  private String m_fileToOpen;

  public CambioActivity()
  {
    m_instance = this;
  }
	  

  public static void openFileIfNeedBe()
  {
    if( m_instance == null || m_instance.m_fileToOpen == null )
      return;

    if( m_instance.openFile(m_instance.m_fileToOpen) )
      m_instance.m_fileToOpen = null;
  }//void openFileIfNeedBe()


  @Override
  protected void onNewIntent (Intent intent)
  {
    //This function is called when Cambio is already running (but in the background)
    //  and another app like Google Drive requests it to open up a file.

    setIntent(intent);
    String action = intent.getAction();
    String type = intent.getType();

    if( Intent.ACTION_VIEW.equals(action) )
    {
       Uri fileUri = (Uri) intent.getData();
       if( fileUri != null )
       {
         if( !openFile( fileUri.getPath() ) )
           m_fileToOpen = fileUri.getPath();
         m_fileToOpen = fileUri.getPath();
       }//if( fileUri != null )
    }else if( Intent.ACTION_SEND_MULTIPLE.equals(action) && type != null )
    {
        // Need to handle multiple files here
    } else
    {
        // Handle other intents, such as being started from the home screen
    }//if( decide what actiuon is wanted )
  }//void onNewIntent (Intent intent)


  @Override
  public void onCreate(Bundle savedInstanceState)
  {
    //This function is called on creatin of the app
    super.onCreate(savedInstanceState);
    
    Intent intent = getIntent();
    String action = intent.getAction();
    String type = intent.getType();

    if (Intent.ACTION_VIEW.equals(action) ) 
    {
       Uri fileUri = (Uri) intent.getData();
       if (fileUri != null)
       {
         if( !openFile( fileUri.getPath() ) )
           m_fileToOpen = fileUri.getPath();
         m_fileToOpen = fileUri.getPath();
       }
    } else if (Intent.ACTION_SEND_MULTIPLE.equals(action) && type != null) 
    {
        // Need to handle multiple files here
    } else 
    {
        // Handle other intents, such as being started from the home screen
    }
    
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
  }

  public static native boolean openFile( String path  );
}
