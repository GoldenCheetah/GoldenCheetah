project (libkml)

cmake_minimum_required(VERSION 3.4)

# Download (git version), configure, build, and install liblibkml.
#
# NB. one have to type passsword (sudo) to install it in /usr/local

include(ExternalProject)

set (LIBKML_SRC_DIR libkml-src)

ExternalProject_Add(libkml-git
  GIT_REPOSITORY https://github.com/libkml/libkml.git
  GIT_TAG 1.3.0
  SOURCE_DIR ${LIBKML_SRC_DIR}
  #BUILD_IN_SOURCE 1
  #CONFIGURE_COMMAND ./configure
  CMAKE_ARGS -DCMAKE_BUILD_TYPE:STRING=Release
  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND sudo $(MAKE) install 
  )

#ExternalProject_Add_Step(libkml-git gen-configure-script
#  COMMAND cd ${LIBKML_SRC_DIR} && sh autogen.sh
#  COMMENT "Generating configure script... "     # Text printed when step executes
#  DEPENDEES download    # Steps on which this step depends
#  DEPENDERS configure    # Steps that depend on this step
#  #[DEPENDS files...]      # Files on which this step depends
#  #[ALWAYS 1]              # No stamp file, step always runs
#  #WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
#  #[LOG 1]                 # Wrap step in script to log output
#  )
