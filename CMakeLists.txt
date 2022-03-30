cmake_minimum_required(VERSION 3.15 FATAL_ERROR)

project(cambio)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

#Cambio can be built as a GUI program, and/or a command line executable to
#  convert file formats.
option( BUILD_CAMBIO_GUI "Builds the GUI component of Cambio" ON )
option( BUILD_CAMBIO_COMMAND_LINE "Builds the command line component of Cambio" ON )
option( SHOW_CAMBIO_SPLASH_SCREEN "Show splash screen on application start" OFF )
option( BUILD_TEMPLATE_REGRESSION_TEST "Creates executuable to perform template engine regression tests" OFF )

set( Cambio_VERSION Development CACHE STRING "Cambio Version" )


if( BUILD_CAMBIO_GUI )
    include( cmake/addQtProject.cmake )
endif( BUILD_CAMBIO_GUI )

IF(NOT CMAKE_BUILD_TYPE)
    SET( CMAKE_BUILD_TYPE Debug CACHE STRING "Build type: Debug Release RelWithDebInfo MinSizeRel." FORCE )
ENDIF(NOT CMAKE_BUILD_TYPE)

if(WIN32)
    if( NOT MyQT_DIR )
      set( MyQT_DIR "C:/install/msvc2017/x64/Qt_5.12_static" )
    endif()

    if( NOT BOOST_ROOT )
      set( BOOST_ROOT "C:/install/msvc2017/x64/boost_1_65_1" )
    endif()

    set( BOOST_PREFIX ${BOOST_ROOT} )
   
    set( Boost_USE_STATIC_RUNTIME ON )
    set( CMAKE_CXX_FLAGS "/MT ${CMAKE_CXX_FLAGS}" )
  	set(CompilerFlags
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO
        )
    foreach(CompilerFlag ${CompilerFlags})
        string(REGEX REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()

	#Set multiprocess build and to target Win7 SP1 (0x0601==Win7, 0x0501==WinXP)
	set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /DWINVER=0x0601 /D_WIN32_WINNT=0x0601" )

	# Lets make sure we can run on Win7 still - not entirely sure how to do this correctly
	#set( WIN_SDK_VERSION "8.1" ) #"10.0.15063.0"
  #set( CMAKE_SYSTEM_VERSION "${WIN_SDK_VERSION}" )
	#set( CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM "${WIN_SDK_VERSION}" )
endif()
 
if( APPLE AND BUILD_CAMBIO_GUI )
    add_definitions( -DQT_STATICPLUGIN )
    if( NOT MyQT_DIR )
      set( MyQT_DIR "/Users/wcjohns/install/Qt5.11.1_static" )
    endif()
endif()

if( ANDROID )
    #not sure why Android specifically requires specifying the boost compiler...
    set( Boost_COMPILER "-gcc" )
    set( MyQT_DIR "/Users/wcjohns/install/Qt/5.3/android_armv7" )
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if( Boost_USE_STATIC_LIBS )
  set( CMAKE_FIND_LIBRARY_SUFFIXES .a .la .lib )
endif()


set( headers ${headers} cambio/Cambio_config.h.in )

if( BUILD_CAMBIO_GUI )
  # Instruct CMake to run moc automatically when needed.
  set( CMAKE_AUTOMOC ON )

  # Find the QtWidgets library
  set( CMAKE_PREFIX_PATH ${MyQT_DIR} )
  set( CMAKE_PREFIX_PATH "${MyQT_DIR}/lib/cmake/Qt5" )

  find_package( Qt5 COMPONENTS Core Widgets Charts PrintSupport REQUIRED )

  if(WIN32)
    find_package(Qt5 COMPONENTS WinExtras REQUIRED)
  elseif( APPLE )
    find_package(Qt5 COMPONENTS MacExtras REQUIRED)
  endif()

  IF( WIN32 )
    SET( CMAKE_MFC_FLAG "0" )
    set( cambio_ICON "target/windows/cambio.rc" )
    SET( GUI_TYPE WIN32 "target/windows/cambio.rc" "${cambio_ICON}" )
  ENDIF( WIN32 )

  IF( APPLE )
    set( cambio_ICON ${CMAKE_SOURCE_DIR}/target/osx/cambioicon.icns )
    SET( GUI_TYPE MACOSX_BUNDLE "${cambio_ICON}" )
  ENDIF( APPLE )

  set( headers
       ${headers}
       cambio/TimeView.h
       cambio/TimeChart.h
       cambio/CambioApp.h
       cambio/MainWindow.h
       cambio/SaveWidget.h
       cambio/SpectrumView.h
       cambio/SpectrumChart.h
       cambio/FileDetailWidget.h
       cambio/FileDetailTools.h
       cambio/BatchConvertDialog.h
       cambio/BusyIndicator.h
       cambio/AxisLabelUtils.h
       cambio/sandia_logo.h
)

  set( sources
       ${sources}
       src/TimeView.cpp
       src/TimeChart.cpp
       src/CambioApp.cpp
       src/MainWindow.cpp
       src/SaveWidget.cpp
       src/SpectrumView.cpp
       src/SpectrumChart.cpp
       src/FileDetailWidget.cpp
       src/FileDetailTools.cpp
       src/BatchConvertDialog.cpp
       src/BusyIndicator.cpp
       src/AxisLabelUtils.cpp
       src/sandia_logo.cpp
)
else( BUILD_CAMBIO_GUI )
  
endif( BUILD_CAMBIO_GUI )


if( APPLE AND BUILD_CAMBIO_GUI )
  set( headers ${headers} cambio/macos_helper.h )
  set( sources ${sources} src/macos_helper.mm )
endif( APPLE AND BUILD_CAMBIO_GUI )

if( BUILD_CAMBIO_COMMAND_LINE )
  set( headers ${headers} cambio/CommandLineUtil.h )
  set( sources ${sources} src/CommandLineUtil.cpp )
  find_library( BOOST_PROGRAM_OPTIONS boost_program_options REQUIRED)
endif( BUILD_CAMBIO_COMMAND_LINE )

if( NOT BUILD_CAMBIO_GUI AND NOT BUILD_CAMBIO_COMMAND_LINE )
  message( FATAL_ERROR "You must select at at least one of BUILD_CAMBIO_GUI or BUILD_CAMBIO_COMMAND_LINE" )
endif( NOT BUILD_CAMBIO_GUI AND NOT BUILD_CAMBIO_COMMAND_LINE )


add_executable( ${PROJECT_NAME} ${GUI_TYPE} main.cpp ${sources} ${headers} )

if( BUILD_CAMBIO_COMMAND_LINE )
  target_link_libraries( ${PROJECT_NAME} PRIVATE ${BOOST_PROGRAM_OPTIONS} )
  
  if(WIN32)
    set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS "wsetargv.obj")
  endif(WIN32)
endif()

if( NOT BUILD_CAMBIO_GUI )
  if( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
    #Right now we are copying all of libpthread into our executable to force copying weak symbols in.
    #  Instead, to save size should specify the functions we actually need using '-u pthread_create', etc.
    #  Could maybe add a "-no_weak_exports" flag to the linker as well to make sure none missing.
    set( CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-pthread -Wl,--whole-archive -lpthread -Wl,--no-whole-archive")
    target_link_libraries( ${PROJECT_NAME} PRIVATE -static-libgcc -static-libstdc++ )
  endif( ${CMAKE_SYSTEM_NAME} STREQUAL "Linux" )
endif( NOT BUILD_CAMBIO_GUI )

IF( APPLE AND NOT IOS AND BUILD_CAMBIO_GUI )
  set( CAMBIO_VERSION "2.1" )
  set( CAMBIO_VERSION_NUM "2" )
  #http://www.cmake.org/Wiki/CMake:Bundles_And_Frameworks
  set_target_properties( ${PROJECT_NAME} PROPERTIES
                         MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/target/osx/Info.plist.cmake"
  )

  set( MACOSX_BUNDLE_INFO_STRING "Cambio - Sandia National Labs, Will Johnson" )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER "gov.sandia.macOS.cambio" )
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "Cambio" )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING "Cambio" )
  set( MACOSX_BUNDLE_BUNDLE_VERSION "0" )
  set( MACOSX_BUNDLE_COPYRIGHT "Sandia National Labs, Will Johnson" )
  set( MACOSX_BUNDLE_ICON_FILE cambioicon.icns )
  set_source_files_properties( ${cambio_ICON} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources" )

  set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
    "${CMAKE_CURRENT_SOURCE_DIR}/target/osx/cambio.entitlements")

  # With out this next line, Xcode will (silently) fail to build Archive builds
  set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_INSTALL_PATH "/Applications")

  #Figure out the development team ID, and set this info for Xcode so it will default to signing the app
  #  This is probably a little brittle on other peoples machined
  execute_process(COMMAND /usr/bin/security find-identity -v -p codesigning OUTPUT_VARIABLE dev_team_ids )
  STRING(REGEX REPLACE ";" "\\\\;" dev_team_ids "${dev_team_ids}")
  STRING(REGEX REPLACE "\n" ";" dev_team_ids "${dev_team_ids}")
  LIST(FILTER dev_team_ids INCLUDE REGEX "[Developer ID Application:.+|Apple Development.+]" )
  message( "dev_team_ids = ${dev_team_ids}")
  STRING(REGEX MATCH "\\(.+\\)" mac_team_id "${dev_team_ids}" )
  STRING(REGEX REPLACE "\\(|\\)" "" mac_team_id "${mac_team_id}")
  message( "Using Mac Developer Team ID='${mac_team_id}'" )

  set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer")
  set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${mac_team_id}" )
  set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME "YES" )
ENDIF()


# Use the Widgets module from Qt 5.
if( BUILD_CAMBIO_GUI )
  if(WIN32)
    target_link_libraries( ${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets Qt5::Charts Qt5::PrintSupport Qt5::WinExtras )

    #Note, Qt CMake doesnt pull in the required additional libraries for statically
    #  linking, so we have to do this manually (libraries determined by trial and error)
	  set( LIB_TYPE_POSTFIX "" )
      if( CMAKE_BUILD_TYPE EQUAL "DEBUG" )
	    set( LIB_TYPE_POSTFIX "d" )
	  endif()

	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\Qt5EventDispatcherSupport${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\Qt5ThemeSupport${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\Qt5WindowsUIAutomationSupport${LIB_TYPE_POSTFIX}.lib" )
    
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\qtmain${LIB_TYPE_POSTFIX}.lib"  )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\qtlibpng${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\qtharfbuzz${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\Qt5FontDatabaseSupport${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\qtpcre2${LIB_TYPE_POSTFIX}.lib" )
	  target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\lib\\qtfreetype${LIB_TYPE_POSTFIX}.lib" )

    target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}\\plugins\\platforms\\qwindows${LIB_TYPE_POSTFIX}.lib"  )
	
	  #Print support needed by qcustomplot
	  #target_link_libraries( ${PROJECT_NAME} "${MyQT_DIR}\\lib\\Qt5PrintSupport.lib"  )
	  #target_link_libraries( ${PROJECT_NAME} "${MyQT_DIR}\\plugins\\printsupport\\windowsprintersupport.lib"  )

    target_link_libraries( ${PROJECT_NAME} PRIVATE "imm32.lib" "Ws2_32.lib" "winmm.lib" "Mincore.lib" "Dwmapi.lib" "Wtsapi32.lib" "Pathcch.lib" "Shlwapi.lib" "Version.lib" )
  else()
    link_directories( ${MyQT_DIR}/lib )
    link_directories( ${MyQT_DIR}/plugins/platforms )

    target_link_libraries( ${PROJECT_NAME} PRIVATE Qt5::Core Qt5::Widgets Qt5::Charts Qt5::PrintSupport )

    if( APPLE AND NOT IOS )
      target_link_libraries( ${PROJECT_NAME} PRIVATE Qt5::MacExtras )
    endif( APPLE AND NOT IOS )


    IF( APPLE AND BUILD_CAMBIO_GUI )
      find_library( COCOA_LIBRARY Cocoa )
      find_library( CARBON_LIBRARY Carbon )
      find_library( CORE_SERVICES_LIBRARY CoreServices )
      find_library( OpenGL_LIBRARY OpenGL )
      find_library( IO_KIT_LIBRARY IOKit )
      find_library( APP_SERVICES_LIBRARY ApplicationServices )
      find_library( CORE_FOUNDATION_LIBRARY CoreFoundation )
      find_library( QUARTZ_LIBRARY Quartz )
      find_library( FOUNDATION_LIBRARY Foundation )
      find_library( SECURITY_LIBRARY Security )  

      mark_as_advanced( COCOA_LIBRARY CARBON_LIBRARY CORE_SERVICES_LIBRARY
                       APP_SERVICES_LIBRARY CORE_FOUNDATION_LIBRARY
                       FOUNDATION_LIBRARY IO_KIT_LIBRARY OpenGL_LIBRARY
                       SECURITY_LIBRARY QUARTZ_LIBRARY )

      target_link_libraries( ${PROJECT_NAME} PRIVATE
        ${COCOA_LIBRARY} ${CARBON_LIBRARY} ${CORE_SERVICES_LIBRARY}
        ${APP_SERVICES_LIBRARY} ${CORE_FOUNDATION_LIBRARY}
        ${FOUNDATION_LIBRARY} ${IO_KIT_LIBRARY} ${OpenGL_LIBRARY}
        ${SECURITY_LIBRARY} ${QUARTZ_LIBRARY}
        "/usr/lib/libz.1.dylib"
      )

      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/plugins/platforms/libqcocoa.a"  )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/plugins/styles/libqmacstyle.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5MacExtras.a"  )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libqtharfbuzz.a"  )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libqtlibpng.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libqtpcre2.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5ClipboardSupport.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5FontDatabaseSupport.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libqtfreetype.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5GraphicsSupport.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5AccessibilitySupport.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5ThemeSupport.a" )
      target_link_libraries( ${PROJECT_NAME} PRIVATE "/usr/lib/libcups.dylib"  )

#      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5PrintSupport.a" )
#      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5PlatformSupport.a"  )
#      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/lib/libQt5PlatformCompositorSupport.a" )
#      target_link_libraries( ${PROJECT_NAME} PRIVATE "${MyQT_DIR}/plugins/printsupport/libcocoaprintersupport.a"  )
    ENDIF( APPLE AND BUILD_CAMBIO_GUI )
  endif()
endif( BUILD_CAMBIO_GUI )


if( SPECTRUM_DATA_STRUCTS_NO_BOOST_LIB )
  find_path( Boost_INCLUDE_DIR boost/config.hpp
             "/Users/wcjohns/install/boost_1_56_0_osx/include" )
else( SPECTRUM_DATA_STRUCTS_NO_BOOST_LIB )
  if( BUILD_CAMBIO_COMMAND_LINE )
    find_package( Boost 1.44.0 COMPONENTS system filesystem date_time program_options )
  else( BUILD_CAMBIO_COMMAND_LINE )
    find_package( Boost REQUIRED COMPONENTS system filesystem date_time )
  endif( BUILD_CAMBIO_COMMAND_LINE )
endif( SPECTRUM_DATA_STRUCTS_NO_BOOST_LIB )


add_subdirectory( SpecUtils )

target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR} )

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/cambio/Cambio_config.h.in
  ${CMAKE_BINARY_DIR}/Cambio_config.h )

target_link_libraries( ${PROJECT_NAME} PRIVATE Threads::Threads ${Boost_LIBRARIES} SpecUtils )

#Generate pro files so we can use Qt Creator to build apps for Android
#  and iOS (a work in progress)
if( ANDROID )
#  ADD_QT_PROJECT( ${PROJECT_NAME} "" "CambioAndroid.pro" )
endif()

if( IOS )
  ADD_QT_PROJECT( ${PROJECT_NAME} "" "CambioIOS.pro" )
endif()

if( SpecUtils_INJA_TEMPLATES )
	if( BUILD_TEMPLATE_REGRESSION_TEST )
	  add_subdirectory( template_test )
	endif( BUILD_TEMPLATE_REGRESSION_TEST )
endif( SpecUtils_INJA_TEMPLATES )
