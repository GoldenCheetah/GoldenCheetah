
# Copyright (c) 2018 Fredik Lingvall (fredrik.lingvall@protonmail.com)

# LIBVLC_INCLUDE_DIR = libvlc.h
# LIBVLC_LIBRARIES = libvlc.so
# LIBVLC_FOUND = true if LIBVLC is found

if (WIN32)
  set(CMAKE_PREFIX_PATH "C:/LIBVLC")
endif(WIN32)

find_path (LIBVLC_INCLUDE_DIR libvlc.h PATH_SUFFIXES vlc)
if (NOT LIBVLC_INCLUDE_DIR)
  message (STATUS "Could not find libvlc.h")
endif (NOT LIBVLC_INCLUDE_DIR)

find_library (LIBVLC_LIBRARIES NAMES libvlc vlc PATH_SUFFIXES lib)
if (NOT LIBVLC_LIBRARIES)
  message (STATUS "Could not find LIBVLC library")
endif (NOT LIBVLC_LIBRARIES)

if (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARIES)
  set (LIBVLC_FOUND TRUE)
endif (LIBVLC_INCLUDE_DIR AND LIBVLC_LIBRARIES)

if (LIBVLC_FOUND)

  if (NOT LIBVLC_FIND_QUIETLY)
    message (STATUS "Found libvlc.h: ${LIBVLC_INCLUDE_DIR}")
  endif (NOT LIBVLC_FIND_QUIETLY)

  if (NOT LIBVLC_FIND_QUIETLY)
    message (STATUS "Found LIBVLC: ${LIBVLC_LIBRARIES}")
  endif (NOT LIBVLC_FIND_QUIETLY)

else (LIBVLC_FOUND)
  if (LIBVLC_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find LIBVLC")
  endif (LIBVLC_FIND_REQUIRED)
endif (LIBVLC_FOUND)

mark_as_advanced (LIBVLC_INCLUDE_DIR LIBVLC_LIBRARIES LIBVLC_FOUND)
