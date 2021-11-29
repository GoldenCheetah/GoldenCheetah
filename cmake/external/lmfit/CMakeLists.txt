project (lmfit)

cmake_minimum_required(VERSION 3.4)

# Download, configure, build, and install lmfit.
#
# NB. one have to type passsword (sudo) to install it in /usr/local

message (STATUS "Building lmfit")

include(ExternalProject)

#set (LMFIT_SRC_DIR lmfit-src)

ExternalProject_Add(lmfit-7.0
  URL http://apps.jcns.fz-juelich.de/src/lmfit/lmfit-7.0.tgz
  DOWNLOAD_DIR download
  #SOURCE_DIR ${LMFIT_SRC_DIR}
  SOURCE_DIR lmfit-src
  CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX="/usr/local"
  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND sudo $(MAKE) install 
  )
