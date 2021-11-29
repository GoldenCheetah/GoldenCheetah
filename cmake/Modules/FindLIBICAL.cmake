
# Copyright (c) 2018 Fredik Lingvall (fredrik.lingvall@protonmail.com)
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc., 51
# Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

# LIBICAL_INCLUDE_DIR = ical.h
# LIBICAL_LIBRARIES = libical.so
# LIBICAL_FOUND = true if LIBICAL is found

if (WIN32)
  set(CMAKE_PREFIX_PATH "C:/LIBICAL")
endif(WIN32)

find_path (LIBICAL_INCLUDE_DIR ical.h PATH_SUFFIXES libical)
if (NOT LIBICAL_INCLUDE_DIR)
  message (STATUS "Could not find ical.h")
endif (NOT LIBICAL_INCLUDE_DIR)

find_library (LIBICAL_LIBRARIES NAMES libical ical PATH_SUFFIXES lib)
if (NOT LIBICAL_LIBRARIES)
  message (STATUS "Could not find LIBICAL library")
endif (NOT LIBICAL_LIBRARIES)

if (LIBICAL_INCLUDE_DIR AND LIBICAL_LIBRARIES)
  set (LIBICAL_FOUND TRUE)
endif (LIBICAL_INCLUDE_DIR AND LIBICAL_LIBRARIES)

if (LIBICAL_FOUND)

  if (NOT LIBICAL_FIND_QUIETLY)
    message (STATUS "Found ical.h: ${LIBICAL_INCLUDE_DIR}")
  endif (NOT LIBICAL_FIND_QUIETLY)

  if (NOT LIBICAL_FIND_QUIETLY)
    message (STATUS "Found LIBICAL: ${LIBICAL_LIBRARIES}")
  endif (NOT LIBICAL_FIND_QUIETLY)

else (LIBICAL_FOUND)
  if (LIBICAL_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find LIBICAL")
  endif (LIBICAL_FIND_REQUIRED)
endif (LIBICAL_FOUND)

mark_as_advanced (LIBICAL_INCLUDE_DIR LIBICAL_LIBRARIES LIBICAL_FOUND)
