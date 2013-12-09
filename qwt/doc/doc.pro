################################################################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
################################################################

# qmake project file for installing the documentation

QWT_ROOT = $${PWD}/..
include( $${QWT_ROOT}/qwtconfig.pri )

TEMPLATE = subdirs

doc.files      =  $${QWT_ROOT}/doc/html
unix:doc.files += $${QWT_ROOT}/doc/man
doc.path       =  $${QWT_INSTALL_DOCS}

INSTALLS       = doc

