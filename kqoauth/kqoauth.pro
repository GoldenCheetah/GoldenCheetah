TARGET = kqoauth
DESTDIR = .
win32:DLLDESTDIR = $${DESTDIR}

VERSION = 0.97

TEMPLATE = lib
QT += network
CONFIG += \
    create_prl

unix:!macx: CONFIG += static

INC_DIR = .

LIBS += -lssl -lcrypto

INCLUDEPATH += .

PUBLIC_HEADERS += kqoauthmanager.h \
                  kqoauthrequest.h \
                  kqoauthrequest_1.h \
                  kqoauthrequest_xauth.h \
                  kqoauthglobals.h 

PRIVATE_HEADERS +=  kqoauthrequest_p.h \
                    kqoauthmanager_p.h \
                    kqoauthauthreplyserver.h \
                    kqoauthauthreplyserver_p.h \
                    kqoauthutils.h \
                    kqoauthrequest_xauth_p.h

HEADERS = \
    $$PUBLIC_HEADERS \
    $$PRIVATE_HEADERS 

SOURCES += \ 
    kqoauthmanager.cpp \
    kqoauthrequest.cpp \
    kqoauthutils.cpp \
    kqoauthauthreplyserver.cpp \
    kqoauthrequest_1.cpp \
    kqoauthrequest_xauth.cpp

DEFINES += KQOAUTH

headers.files = \
    $${PUBLIC_HEADERS} \
    $${INC_DIR}/QtKOAuth
features.path = $$[QMAKE_MKSPECS]/features
features.files = ../kqoauth.prf
docs.files = ../doc/html

macx {
    CONFIG += lib_bundle
    QMAKE_FRAMEWORK_BUNDLE_NAME = $$TARGET
    CONFIG(debug, debug|release): CONFIG += build_all
    FRAMEWORK_HEADERS.version = Versions
    FRAMEWORK_HEADERS.files = $$headers.files
    FRAMEWORK_HEADERS.path = Headers
    QMAKE_BUNDLE_DATA += FRAMEWORK_HEADERS
    target.path = $$[QT_INSTALL_LIBS]
    INSTALLS += \
        target \
        features \
        postinstall
    postinstall.path =  target.path
    postinstall.extra = install_name_tool -id $${target.path}/$${QMAKE_FRAMEWORK_BUNDLE_NAME}.framework/Versions/0/$${TARGET} $${target.path}/$${QMAKE_FRAMEWORK_BUNDLE_NAME}.framework/Versions/0/$${TARGET}
}
else:unix {
    isEmpty( PREFIX ):INSTALL_PREFIX = /usr
    else:INSTALL_PREFIX = $${PREFIX}

    # this creates a pkgconfig file
    system( ./pcfile.sh $${INSTALL_PREFIX} $${VERSION} )
    pkgconfig.files = kqoauth.pc
    
    target.path = $$[QT_INSTALL_LIBS]

    headers.path = $${INSTALL_PREFIX}/include/QtKOAuth
    docs.path = $${INSTALL_PREFIX}/share/doc/$${TARGET}-$${VERSION}/html
    pkgconfig.path = $${target.path}/pkgconfig
    INSTALLS += \
        target \
        headers \
        docs \
        pkgconfig \
        features
}
else:windows {
    INSTALL_PREFIX = $$[QT_INSTALL_PREFIX]
    target.path = $${INSTALL_PREFIX}/lib
    headers.path = $${INSTALL_PREFIX}/include/QtKOAuth
    INSTALLS += \
        target \
        headers \
        features
}

CONFIG(debug_and_release) {
    build_pass:CONFIG(debug, debug|release) {
        unix: TARGET = $$join(TARGET,,,_debug)
        else: TARGET = $$join(TARGET,,,d)
    }
}
