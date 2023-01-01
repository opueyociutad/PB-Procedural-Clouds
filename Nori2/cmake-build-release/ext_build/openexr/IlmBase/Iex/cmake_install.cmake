# Install script for directory: /home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/cmake-build-release/ext_build/openexr/IlmBase/Iex/libIex.a")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OpenEXR" TYPE FILE FILES
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexBaseExc.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexMathExc.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexThrowErrnoExc.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexErrnoExc.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexMacros.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/Iex.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexNamespace.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexExport.h"
    "/home/oscar/Documents/MRGCV/MSoA-22-23/Nori2/ext/openexr/IlmBase/Iex/IexForward.h"
    )
endif()

