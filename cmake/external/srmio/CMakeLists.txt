project (srmio)

cmake_minimum_required(VERSION 3.4)

# Download (git version), configure, build, and install libsrmio.
#
# NB. one have to type passsword (sudo) to install it in /usr/local

include(ExternalProject)

set (SRMIO_SRC_DIR srmio-src)

ExternalProject_Add(srmio-git
  GIT_REPOSITORY https://github.com/rclasen/srmio.git
  # Latest tag is from Sun Jun 23 14:28:21 2013 (!?)
  #GIT_TAG v0.1.1git1
  SOURCE_DIR ${SRMIO_SRC_DIR}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND ./configure
  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND sudo $(MAKE) install 
  )

ExternalProject_Add_Step(srmio-git gen-configure-script
  COMMAND cd ${SRMIO_SRC_DIR} && sh genautomake.sh
  COMMENT "Generating configure script... "     # Text printed when step executes
  DEPENDEES download    # Steps on which this step depends
  DEPENDERS configure    # Steps that depend on this step
  #[DEPENDS files...]      # Files on which this step depends
  #[ALWAYS 1]              # No stamp file, step always runs
  #WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  #[LOG 1]                 # Wrap step in script to log output
  )
