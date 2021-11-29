
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

# KQOAUTH_INCLUDE_DIR = ical.h
# KQOAUTH_LIBRARIES = kqoauth.so
# KQOAUTH_FOUND = true if KQOAUTH is found

if (WIN32)
  set(CMAKE_PREFIX_PATH "C:/KQOAUTH")
endif(WIN32)

find_path (KQOAUTH_INCLUDE_DIR kqoauthmanager.h PATH_SUFFIXES QtKOAuth)
if (NOT KQOAUTH_INCLUDE_DIR)
  message (STATUS "Could not find kqoauthmanager.h")
endif (NOT KQOAUTH_INCLUDE_DIR)

find_library (KQOAUTH_LIBRARIES NAMES libkqoauth-qt5 kqoauth-qt5 PATH_SUFFIXES lib)
if (NOT KQOAUTH_LIBRARIES)
  message (STATUS "Could not find KQOAUTH library")
endif (NOT KQOAUTH_LIBRARIES)

if (KQOAUTH_INCLUDE_DIR AND KQOAUTH_LIBRARIES)
  set (KQOAUTH_FOUND TRUE)
endif (KQOAUTH_INCLUDE_DIR AND KQOAUTH_LIBRARIES)

if (KQOAUTH_FOUND)

  if (NOT KQOAUTH_FIND_QUIETLY)
    message (STATUS "Found kqoauthmanager.h: ${KQOAUTH_INCLUDE_DIR}")
  endif (NOT KQOAUTH_FIND_QUIETLY)

  if (NOT KQOAUTH_FIND_QUIETLY)
    message (STATUS "Found KQOAUTH: ${KQOAUTH_LIBRARIES}")
  endif (NOT KQOAUTH_FIND_QUIETLY)

else (KQOAUTH_FOUND)
  if (KQOAUTH_FIND_REQUIRED)
    message (FATAL_ERROR "Could not find KQOAUTH")
  endif (KQOAUTH_FIND_REQUIRED)
endif (KQOAUTH_FOUND)

mark_as_advanced (KQOAUTH_INCLUDE_DIR KQOAUTH_LIBRARIES KQOAUTH_FOUND)
