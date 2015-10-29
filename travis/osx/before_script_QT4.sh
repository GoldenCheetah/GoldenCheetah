#!/bin/bash

# Copy default files to actual build files

cp qwt/qwtconfig.pri.in qwt/qwtconfig.pri
cp src/gcconfig.pri.in src/gcconfig.pri

# Modify src/gcconfig.pri with build specifics

sed -i "" "s|#\(CONFIG += release.*\)|\1 static |" src/gcconfig.pri
sed -i "" "s|#\(QMAKE_LFLAGS\).*|\1_RELEASE += -mmacosx-version-min=10.7 |" src/gcconfig.pri
sed -i "" "s|#\(SRMIO_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(D2XX_INCLUDE =.*\)|\1 ../D2XX|" src/gcconfig.pri
sed -i "" "s|#\(D2XX_LIBS    =.*\)|\1 -L../D2XX -lftd2xx.1.2.2|" src/gcconfig.pri
sed -i "" "s|#\(KQOAUTH_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(KQOAUTH_INCLUDE =.*\)|\1 \$\$[QT_INSTALL_LIBS]/kqoauth.framework/Headers|" src/gcconfig.pri
sed -i "" "s|#\(KQOAUTH_LIBS =.*\)|\1 -F\$\$[QT_INSTALL_LIBS] -framework kqoauth|" src/gcconfig.pri
sed -i "" "s|#\(QWT3D_INSTALL =.*\)|\1 ../../qwtplot3d|" src/gcconfig.pri
sed -i "" "s|#\(QWT3D_INCLUDE =.*\)|\1 /usr/local/include/qwtplot3d-$QT|" src/gcconfig.pri
sed -i "" "s|#\(QWT3D_LIBS    =.*\)|\1 -L/usr/local/lib -lqwtplot3d-$QT|" src/gcconfig.pri
sed -i "" "s|#\(ICAL_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(ICAL_LIBS    =.*\)|\1 -L/usr/local/lib -lical|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_INSTALL =\).*|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(LIBUSB_LIBS    =.*\)|\1 -L/usr/local/lib -lusb|" src/gcconfig.pri
sed -i "" "s|#\(KML_INSTALL =.*\)|\1 /usr/local|" src/gcconfig.pri
sed -i "" "s|#\(KML_LIBS    =.*\)|\1 -L/usr/local/lib -lkmlxsd -lkmlregionator -lkmldom -lkmlconvenience -lkmlengine -lkmlbase|" src/gcconfig.pri
sed -i "" "s|#\(QMAKE_CFLAGS_\).*|\1RELEASE += -mmacosx-version-min=10.7 -arch x86_64|" src/gcconfig.pri
sed -i "" "s|#\(QMAKE_CXXFLAGS_\).*|\1RELEASE += -mmacosx-version-min=10.7 -arch x86_64|" src/gcconfig.pri
sed -i "" "s|#\(DEFINES += GC_VIDEO_QUICKTIME.*\)|\1 |" src/gcconfig.pri
sed -i "" "s|#\(CONFIG += link_pkgconfig\)|\1|" src/gcconfig.pri
sed -i "" "s|#\(PKGCONFIG = .*\)|\1 libical libusb|" src/gcconfig.pri

# Allow D2XX to be included in the deployment bundle

sed -i "" "s|libftd2xx.dylib|@executable_path/../Frameworks/libftd2xx.1.2.2.dylib|" src/D2XX.cpp

# Update translations
/usr/local/opt/qt/bin/lupdate src/src.pro

exit
