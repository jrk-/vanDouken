
project(hpiif)

cmake_minimum_required(VERSION 2.8)

if(NOT HPX_ROOT)
    if(NOT $ENV{HPX_ROOT} STREQUAL "")
        set(HPX_ROOT $ENV{HPX_ROOT})
    endif()
endif()

if(HPX_ROOT)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${HPX_ROOT}/share/cmake-2.8/Modules)
endif()

find_package(HPX)

set(BOOST_LIBRARIES
    date_time
    iostreams
    filesystem
    program_options
    regex
    serialization
    system
    thread
    chrono)
find_package(HPX_Boost)

find_package(Qt4 REQUIRED QtOpenGL QtGui QtCore)

include(${QT_USE_FILE})

set(HEADERS ${HEADERS}
  ${RELATIVE_PATH}canvascell.h
  ${RELATIVE_PATH}canvasinitializer.h
  ${RELATIVE_PATH}canvaswriter.h
  ${RELATIVE_PATH}flowwidget.h
  ${RELATIVE_PATH}hpx_updategroup.hpp
)

# fixme: rework this
set(MY_HEADERS
model_test/particlewidget.h
flowingcanvas_gui.hpp
server/collector.hpp
)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${HPX_CXX_COMPILE_FLAGS}")

add_definitions(-DQT_NO_KEYWORDS)

QT4_WRAP_CPP(MY_MOC_SOURCES ${MY_HEADERS} OPTIONS -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
include_directories(./)
include_directories(./model_test)
include_directories(${HPX_INCLUDE_DIR})
include_directories(${LIBGEODECOMP_INCLUDE_DIR})

link_directories(${HPX_LIBRARY_DIR})

set(HPX_RPATH "/home/heller/lib:${HPX_RPATH}:${CMAKE_INSTALL_PREFIX}/lib/hpx:${CMAKE_BINARY_DIR}/lib/hpx")

add_hpx_component(flowingcanvas_components
  ESSENTIAL
  DEPENDENCIES
#    ${LOCAL_LIBGEODECOMP_LINK_LIB}
    ${QT_LIBRARIES}
    ${BOOST_FOUND_LIBRARIES}
    compress_zlib
    compress_snappy
    compress_bzip2
    GLU
  COMPONENT_DEPENDENCIES
    iostreams
  SOURCES
    flowingcanvas_component.cpp
    flowingcanvas_updategroup.cpp
    flowingcanvas_simulator.cpp
    flowingcanvas_collector.cpp
    flowingcanvas_gui.cpp
    ${MY_MOC_SOURCES}
)

add_hpx_executable(flowingcanvas_hpx 
    ESSENTIAL
  SOURCES main.cpp
  DEPENDENCIES
  ${BOOST_FOUND_LIBRARIES}
    compress_zlib
    compress_snappy
    compress_bzip2
    COMPONENT_DEPENDENCIES
    GLU
  flowingcanvas_components
    distributing_factory
    iostreams
    FOLDER "flowingcanvas"
  )

add_hpx_executable(flowingcanvas_gui
  ESSENTIAL
  SOURCES main_gui.cpp
  DEPENDENCIES
#    ${LOCAL_LIBGEODECOMP_LINK_LIB}
    ${QT_LIBRARIES}
    ${BOOST_FOUND_LIBRARIES}
    compress_zlib
    compress_snappy
    compress_bzip2
    GLU
  COMPONENT_DEPENDENCIES
    flowingcanvas_components
    distributing_factory
    iostreams
  FOLDER "flowingcanvas"
)