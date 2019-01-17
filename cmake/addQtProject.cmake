#adapted from http://qt-project.org/forums/viewthread/36359

macro(QMAKE_ADD_LINE file_contents line)
	#I dont actually know what this macro is supposed to look like
	set( ${file_contents} "${${file_contents}}\n${line}" )
endmacro(QMAKE_ADD_LINE)

macro(QMAKE_ADD_FILES file_contents target_sources)
	#I dont actually know what this macro is supposed to look like
	set( ${file_contents} "${${file_contents}}\nSOURCES += " )
	foreach (file ${${target_sources}})
  	  set( ${file_contents} "${${file_contents}} ${file}" )
	endforeach(file)
endmacro(QMAKE_ADD_FILES)

macro(QMAKE_ADD_INCLUDE_DIRECTORIES file_contents include_dirs)
	#I dont actually know what this macro is supposed to look like
	set( ${file_contents} "${${file_contents}}\nINCLUDEPATH += " )
	foreach (dir ${${include_dirs}})
  	  set( ${file_contents} "${${file_contents}} ${dir}" )
	endforeach(dir)
endmacro(QMAKE_ADD_INCLUDE_DIRECTORIES)


macro(QMAKE_ADD_LINK_DIRECTORIES file_contents link_dirs)
	#I dont actually know what this macro is supposed to look like
	set( ${file_contents} "${${file_contents}}\nLIBS += " )
	foreach (dir ${${link_dirs}})
  	  set( ${file_contents} "${${file_contents}} -L${dir}" )
	endforeach(dir)
endmacro(QMAKE_ADD_LINK_DIRECTORIES)
	
macro(QMAKE_ADD_TARGET_LINK_LIBRARIES file_contents link_libs)
	#I dont actually know what this macro is supposed to look like
	set( ${file_contents} "${${file_contents}}\nLIBS += " )
	foreach (lib ${${link_libs}})
		
	  string(REGEX MATCH "(.*)Qt5::(.*)" qt5match "${lib}")
	  if( qt5match )
		  message( ".pro file wont link to ${qt5match}" )
	  else()
		  set( ${file_contents} "${${file_contents}} ${lib}" )
	  endif()
	endforeach(lib)
endmacro(QMAKE_ADD_TARGET_LINK_LIBRARIES)


macro(ADD_QT_PROJECT target_name qt_resources out_file_path)
  set(PRO_FILE_CONTENT)
 
  # ADDING CONFIG AND TARGET NAME
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "CONFIG += qt c++11 resources" )
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "TARGET = ${target_name}" )
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "QT += core gui widgets" )
 
  set(TARGET_RC_FILES ${qt_resources})
  QMAKE_ADD_FILES(PRO_FILE_CONTENT TARGET_RC_FILES)
 
  # SETTING UP TARGET TYPE
  get_target_property(TARGET_CMAKE_TYPE ${target_name} TYPE)
  if(${TARGET_CMAKE_TYPE} STREQUAL "EXECUTABLE")
    QMAKE_ADD_LINE(PRO_FILE_CONTENT "TEMPLATE = app")
  else()
    QMAKE_ADD_LINE(PRO_FILE_CONTENT "TEMPLATE = lib")
  endif()
 
  # ADDING LINK SOURCES
  get_target_property(TARGET_SOURCES ${target_name} SOURCES)
  QMAKE_ADD_FILES(PRO_FILE_CONTENT TARGET_SOURCES)
 
  # ADDING INCLUDE DIRECTORIES
  get_target_property(TARGET_INCLUDE_DIRECTORIES ${target_name} INCLUDE_DIRECTORIES)
  QMAKE_ADD_INCLUDE_DIRECTORIES(PRO_FILE_CONTENT TARGET_INCLUDE_DIRECTORIES)  
 
  # ADDING LINK DIRECTORIES
  get_property(TARGET_LINK_DIRECTORIES DIRECTORY ${CMAKE_CURRENT_LIST_DIR} PROPERTY LINK_DIRECTORIES)
  QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_LINK_DIRECTORIES)
 
  get_target_property(TARGET_ARCHIVE_OUTPUT_DIRECTORY         ${target_name} ARCHIVE_OUTPUT_DIRECTORY)
  get_target_property(TARGET_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${target_name} ARCHIVE_OUTPUT_DIRECTORY_DEBUG)
  get_target_property(TARGET_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${target_name} ARCHIVE_OUTPUT_DIRECTORY_RELEASE)
  get_target_property(TARGET_LIBRARY_OUTPUT_DIRECTORY         ${target_name} LIBRARY_OUTPUT_DIRECTORY)
  get_target_property(TARGET_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${target_name} LIBRARY_OUTPUT_DIRECTORY_DEBUG)
  get_target_property(TARGET_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${target_name} LIBRARY_OUTPUT_DIRECTORY_RELEASE)
 
  if(NOT ${TARGET_ARCHIVE_OUTPUT_DIRECTORY_DEBUG}   STREQUAL "TARGET_ARCHIVE_OUTPUT_DIRECTORY_DEBUG-NOTFOUND")
    QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_ARCHIVE_OUTPUT_DIRECTORY_DEBUG)
  endif()
  if(NOT ${TARGET_ARCHIVE_OUTPUT_DIRECTORY_RELEASE} STREQUAL "TARGET_ARCHIVE_OUTPUT_DIRECTORY_RELEASE-NOTFOUND")
    QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_ARCHIVE_OUTPUT_DIRECTORY_RELEASE)
  endif()
  if(NOT ${TARGET_LIBRARY_OUTPUT_DIRECTORY_DEBUG}   STREQUAL "TARGET_LIBRARY_OUTPUT_DIRECTORY_DEBUG-NOTFOUND")
    QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_LIBRARY_OUTPUT_DIRECTORY_DEBUG)
  endif()
  if(NOT ${TARGET_LIBRARY_OUTPUT_DIRECTORY_RELEASE} STREQUAL "TARGET_LIBRARY_OUTPUT_DIRECTORY_RELEASE-NOTFOUND")
     QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_LIBRARY_OUTPUT_DIRECTORY_RELEASE)
  endif()
 
  if(NOT ${TARGET_ARCHIVE_OUTPUT_DIRECTORY} STREQUAL "TARGET_ARCHIVE_OUTPUT_DIRECTORY-NOTFOUND")
    QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_ARCHIVE_OUTPUT_DIRECTORY)
  endif()
  
  if(NOT ${TARGET_LIBRARY_OUTPUT_DIRECTORY} STREQUAL "TARGET_LIBRARY_OUTPUT_DIRECTORY-NOTFOUND")
    QMAKE_ADD_LINK_DIRECTORIES(PRO_FILE_CONTENT TARGET_LIBRARY_OUTPUT_DIRECTORY)
  endif()
 
  # ADDING LINK LIBRARIES
  get_target_property(TARGET_LINK_LIBRARIES ${target_name} LINK_LIBRARIES)
  QMAKE_ADD_TARGET_LINK_LIBRARIES(PRO_FILE_CONTENT TARGET_LINK_LIBRARIES)
 
  #get_target_property(TARGET_COMPILE_FLAGS ${target_name} COMPILE_FLAGS)
  #PRINT_TARGET_PROPERTY(${target_name} COMPILE_FLAGS)
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "DEFINES += SpecUtilsAsLib ANDROID")
  
  
  #get_target_property(TARGET_COMPILE_DEFINITIONS ${target_name} COMPILE_DEFINITIONS)
  #PRINT_TARGET_PROPERTY(${target_name} COMPILE_DEFINITIONS)
  #
  #get_target_property(TARGET_RESOURCE ${target_name} COMPILE_DEFINITIONS_DEBUG)
  #PRINT_TARGET_PROPERTY(${target_name} COMPILE_DEFINITIONS_DEBUG)
  #
  #get_target_property(TARGET_RESOURCE ${target_name} COMPILE_DEFINITIONS_RELEASE)
  #PRINT_TARGET_PROPERTY(${target_name} COMPILE_DEFINITIONS_RELEASE)
  #
  #get_target_property(TARGET_RESOURCE ${target_name} LINK_FLAGS)
  #PRINT_TARGET_PROPERTY(${target_name} LINK_FLAGS)
  #
  #get_target_property(TARGET_RESOURCE ${target_name} LINK_FLAGS_DEBUG)
  #PRINT_TARGET_PROPERTY(${target_name} LINK_FLAGS_DEBUG)
  #
  #get_target_property(TARGET_RESOURCE ${target_name} LINK_FLAGS_RELEASE)
  #PRINT_TARGET_PROPERTY(${target_name} LINK_FLAGS_RELEASE)
  #
  #
  #get_target_property(TARGET_RESOURCE ${target_name} RESOURCE)
  #PRINT_TARGET_PROPERTY(${target_name} RESOURCE)
 
  # RUNTIME
  get_target_property(TARGET_RUNTIME_OUTPUT_DIRECTORY ${target_name} RUNTIME_OUTPUT_DIRECTORY)
  #QMAKE_ADD_LINE(PRO_FILE_CONTENT "DESTDIR = ${TARGET_RUNTIME_OUTPUT_DIRECTORY}")
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "system(cd ${PROJECT_BINARY_DIR} && make -j8)")
  #QMAKE_ADD_LINE(PRO_FILE_CONTENT "# Please do not modify the following two lines. Required for deployment." )
  #QMAKE_ADD_LINE(PRO_FILE_CONTENT "include(qtquick2applicationviewer/qtquick2applicationviewer.pri)")
  #QMAKE_ADD_LINE(PRO_FILE_CONTENT "qtcAddDeployment()")
 
  QMAKE_ADD_LINE(PRO_FILE_CONTENT "")
 
  #message(${PRO_FILE_CONTENT})
  file(WRITE ${out_file_path} ${PRO_FILE_CONTENT} )
 
endmacro(ADD_QT_PROJECT)