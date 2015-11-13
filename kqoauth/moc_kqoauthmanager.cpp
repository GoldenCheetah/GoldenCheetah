/****************************************************************************
** Meta object code from reading C++ file 'kqoauthmanager.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "kqoauthmanager.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kqoauthmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_KQOAuthManager_t {
    QByteArrayData data[27];
    char stringdata[445];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KQOAuthManager_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KQOAuthManager_t qt_meta_stringdata_KQOAuthManager = {
    {
QT_MOC_LITERAL(0, 0, 14), // "KQOAuthManager"
QT_MOC_LITERAL(1, 15, 12), // "requestReady"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 12), // "networkReply"
QT_MOC_LITERAL(4, 42, 22), // "authorizedRequestReady"
QT_MOC_LITERAL(5, 65, 2), // "id"
QT_MOC_LITERAL(6, 68, 26), // "authorizationPageRequested"
QT_MOC_LITERAL(7, 95, 7), // "pageUrl"
QT_MOC_LITERAL(8, 103, 13), // "receivedToken"
QT_MOC_LITERAL(9, 117, 11), // "oauth_token"
QT_MOC_LITERAL(10, 129, 18), // "oauth_token_secret"
QT_MOC_LITERAL(11, 148, 22), // "temporaryTokenReceived"
QT_MOC_LITERAL(12, 171, 21), // "authorizationReceived"
QT_MOC_LITERAL(13, 193, 14), // "oauth_verifier"
QT_MOC_LITERAL(14, 208, 19), // "accessTokenReceived"
QT_MOC_LITERAL(15, 228, 21), // "authorizedRequestDone"
QT_MOC_LITERAL(16, 250, 22), // "onRequestReplyReceived"
QT_MOC_LITERAL(17, 273, 14), // "QNetworkReply*"
QT_MOC_LITERAL(18, 288, 5), // "reply"
QT_MOC_LITERAL(19, 294, 32), // "onAuthorizedRequestReplyReceived"
QT_MOC_LITERAL(20, 327, 22), // "onVerificationReceived"
QT_MOC_LITERAL(21, 350, 26), // "QMultiMap<QString,QString>"
QT_MOC_LITERAL(22, 377, 8), // "response"
QT_MOC_LITERAL(23, 386, 9), // "slotError"
QT_MOC_LITERAL(24, 396, 27), // "QNetworkReply::NetworkError"
QT_MOC_LITERAL(25, 424, 5), // "error"
QT_MOC_LITERAL(26, 430, 14) // "requestTimeout"

    },
    "KQOAuthManager\0requestReady\0\0networkReply\0"
    "authorizedRequestReady\0id\0"
    "authorizationPageRequested\0pageUrl\0"
    "receivedToken\0oauth_token\0oauth_token_secret\0"
    "temporaryTokenReceived\0authorizationReceived\0"
    "oauth_verifier\0accessTokenReceived\0"
    "authorizedRequestDone\0onRequestReplyReceived\0"
    "QNetworkReply*\0reply\0"
    "onAuthorizedRequestReplyReceived\0"
    "onVerificationReceived\0"
    "QMultiMap<QString,QString>\0response\0"
    "slotError\0QNetworkReply::NetworkError\0"
    "error\0requestTimeout"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KQOAuthManager[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x06 /* Public */,
       4,    2,   82,    2, 0x06 /* Public */,
       6,    1,   87,    2, 0x06 /* Public */,
       8,    2,   90,    2, 0x06 /* Public */,
      11,    2,   95,    2, 0x06 /* Public */,
      12,    2,  100,    2, 0x06 /* Public */,
      14,    2,  105,    2, 0x06 /* Public */,
      15,    0,  110,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      16,    1,  111,    2, 0x08 /* Private */,
      19,    1,  114,    2, 0x08 /* Private */,
      20,    1,  117,    2, 0x08 /* Private */,
      23,    1,  120,    2, 0x08 /* Private */,
      26,    0,  123,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void, QMetaType::QByteArray, QMetaType::Int,    3,    5,
    QMetaType::Void, QMetaType::QUrl,    7,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   13,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    9,   10,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, 0x80000000 | 17,   18,
    QMetaType::Void, 0x80000000 | 21,   22,
    QMetaType::Void, 0x80000000 | 24,   25,
    QMetaType::Void,

       0        // eod
};

void KQOAuthManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        KQOAuthManager *_t = static_cast<KQOAuthManager *>(_o);
        switch (_id) {
        case 0: _t->requestReady((*reinterpret_cast< QByteArray(*)>(_a[1]))); break;
        case 1: _t->authorizedRequestReady((*reinterpret_cast< QByteArray(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 2: _t->authorizationPageRequested((*reinterpret_cast< QUrl(*)>(_a[1]))); break;
        case 3: _t->receivedToken((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 4: _t->temporaryTokenReceived((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 5: _t->authorizationReceived((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 6: _t->accessTokenReceived((*reinterpret_cast< QString(*)>(_a[1])),(*reinterpret_cast< QString(*)>(_a[2]))); break;
        case 7: _t->authorizedRequestDone(); break;
        case 8: _t->onRequestReplyReceived((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 9: _t->onAuthorizedRequestReplyReceived((*reinterpret_cast< QNetworkReply*(*)>(_a[1]))); break;
        case 10: _t->onVerificationReceived((*reinterpret_cast< QMultiMap<QString,QString>(*)>(_a[1]))); break;
        case 11: _t->slotError((*reinterpret_cast< QNetworkReply::NetworkError(*)>(_a[1]))); break;
        case 12: _t->requestTimeout(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 8:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QNetworkReply* >(); break;
            }
            break;
        case 9:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QNetworkReply* >(); break;
            }
            break;
        case 11:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QNetworkReply::NetworkError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (KQOAuthManager::*_t)(QByteArray );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::requestReady)) {
                *result = 0;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QByteArray , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::authorizedRequestReady)) {
                *result = 1;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QUrl );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::authorizationPageRequested)) {
                *result = 2;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QString , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::receivedToken)) {
                *result = 3;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QString , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::temporaryTokenReceived)) {
                *result = 4;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QString , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::authorizationReceived)) {
                *result = 5;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)(QString , QString );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::accessTokenReceived)) {
                *result = 6;
            }
        }
        {
            typedef void (KQOAuthManager::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&KQOAuthManager::authorizedRequestDone)) {
                *result = 7;
            }
        }
    }
}

const QMetaObject KQOAuthManager::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_KQOAuthManager.data,
      qt_meta_data_KQOAuthManager,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *KQOAuthManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KQOAuthManager::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_KQOAuthManager.stringdata))
        return static_cast<void*>(const_cast< KQOAuthManager*>(this));
    return QObject::qt_metacast(_clname);
}

int KQOAuthManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void KQOAuthManager::requestReady(QByteArray _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void KQOAuthManager::authorizedRequestReady(QByteArray _t1, int _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void KQOAuthManager::authorizationPageRequested(QUrl _t1)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void KQOAuthManager::receivedToken(QString _t1, QString _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void KQOAuthManager::temporaryTokenReceived(QString _t1, QString _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void KQOAuthManager::authorizationReceived(QString _t1, QString _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void KQOAuthManager::accessTokenReceived(QString _t1, QString _t2)
{
    void *_a[] = { Q_NULLPTR, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void KQOAuthManager::authorizedRequestDone()
{
    QMetaObject::activate(this, &staticMetaObject, 7, Q_NULLPTR);
}
QT_END_MOC_NAMESPACE
