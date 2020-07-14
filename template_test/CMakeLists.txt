cmake_policy(SET CMP0048 NEW)
project(TemplateRegressionTest VERSION 1)

cmake_minimum_required(VERSION 3.1.0 FATAL_ERROR)

find_package( Boost REQUIRED COMPONENTS program_options date_time system filesystem )

if( NOT Boost_FOUND )
  message(FATAL_ERROR "Couldnt Find Boost")
endif( NOT Boost_FOUND )

add_executable( template_test template_test.cpp )

target_include_directories( template_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${Boost_INCLUDE_DIRS} )

target_link_libraries( template_test PRIVATE ${Boost_LIBRARIES} ${Boost_PROGRAM_OPTIONS_LIBRARY} ${Boost_DATE_TIME_LIBRARY} ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} )

set_target_properties( template_test PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES CXX_EXTENSIONS NO )


