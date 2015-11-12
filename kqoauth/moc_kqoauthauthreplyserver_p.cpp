/****************************************************************************
** Meta object code from reading C++ file 'kqoauthauthreplyserver_p.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.4.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "kqoauthauthreplyserver_p.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'kqoauthauthreplyserver_p.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.4.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_KQOAuthAuthReplyServerPrivate_t {
    QByteArrayData data[4];
    char stringdata[65];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_KQOAuthAuthReplyServerPrivate_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_KQOAuthAuthReplyServerPrivate_t qt_meta_stringdata_KQOAuthAuthReplyServerPrivate = {
    {
QT_MOC_LITERAL(0, 0, 29), // "KQOAuthAuthReplyServerPrivate"
QT_MOC_LITERAL(1, 30, 20), // "onIncomingConnection"
QT_MOC_LITERAL(2, 51, 0), // ""
QT_MOC_LITERAL(3, 52, 12) // "onBytesReady"

    },
    "KQOAuthAuthReplyServerPrivate\0"
    "onIncomingConnection\0\0onBytesReady"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_KQOAuthAuthReplyServerPrivate[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   24,    2, 0x0a /* Public */,
       3,    0,   25,    2, 0x0a /* Public */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void KQOAuthAuthReplyServerPrivate::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        KQOAuthAuthReplyServerPrivate *_t = static_cast<KQOAuthAuthReplyServerPrivate *>(_o);
        switch (_id) {
        case 0: _t->onIncomingConnection(); break;
        case 1: _t->onBytesReady(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObject KQOAuthAuthReplyServerPrivate::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_KQOAuthAuthReplyServerPrivate.data,
      qt_meta_data_KQOAuthAuthReplyServerPrivate,  qt_static_metacall, Q_NULLPTR, Q_NULLPTR}
};


const QMetaObject *KQOAuthAuthReplyServerPrivate::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *KQOAuthAuthReplyServerPrivate::qt_metacast(const char *_clname)
{
    if (!_clname) return Q_NULLPTR;
    if (!strcmp(_clname, qt_meta_stringdata_KQOAuthAuthReplyServerPrivate.stringdata))
        return static_cast<void*>(const_cast< KQOAuthAuthReplyServerPrivate*>(this));
    return QObject::qt_metacast(_clname);
}

int KQOAuthAuthReplyServerPrivate::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
