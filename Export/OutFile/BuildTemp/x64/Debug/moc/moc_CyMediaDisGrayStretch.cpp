/****************************************************************************
** Meta object code from reading C++ file 'CyMediaDisGrayStretch.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../CyMedia/CyMediaDis/CyMediaDisGrayStretch.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CyMediaDisGrayStretch.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CyMediaDisGrayStretch_t {
    QByteArrayData data[14];
    char stringdata0[135];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyMediaDisGrayStretch_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyMediaDisGrayStretch_t qt_meta_stringdata_CyMediaDisGrayStretch = {
    {
QT_MOC_LITERAL(0, 0, 21), // "CyMediaDisGrayStretch"
QT_MOC_LITERAL(1, 22, 9), // "needImage"
QT_MOC_LITERAL(2, 32, 0), // ""
QT_MOC_LITERAL(3, 33, 17), // "stretchParaChange"
QT_MOC_LITERAL(4, 51, 14), // "transImageType"
QT_MOC_LITERAL(5, 66, 5), // "index"
QT_MOC_LITERAL(6, 72, 14), // "upStretchRange"
QT_MOC_LITERAL(7, 87, 5), // "start"
QT_MOC_LITERAL(8, 93, 3), // "end"
QT_MOC_LITERAL(9, 97, 10), // "upHisRange"
QT_MOC_LITERAL(10, 108, 4), // "minX"
QT_MOC_LITERAL(11, 113, 4), // "maxX"
QT_MOC_LITERAL(12, 118, 4), // "maxY"
QT_MOC_LITERAL(13, 123, 11) // "upEditRange"

    },
    "CyMediaDisGrayStretch\0needImage\0\0"
    "stretchParaChange\0transImageType\0index\0"
    "upStretchRange\0start\0end\0upHisRange\0"
    "minX\0maxX\0maxY\0upEditRange"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyMediaDisGrayStretch[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    0,   44,    2, 0x06 /* Public */,
       3,    0,   45,    2, 0x06 /* Public */,
       4,    1,   46,    2, 0x06 /* Public */,
       6,    2,   49,    2, 0x06 /* Public */,
       9,    3,   54,    2, 0x06 /* Public */,
      13,    0,   61,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    7,    8,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,   10,   11,   12,
    QMetaType::Void,

       0        // eod
};

void CyMediaDisGrayStretch::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CyMediaDisGrayStretch *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->needImage(); break;
        case 1: _t->stretchParaChange(); break;
        case 2: _t->transImageType((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->upStretchRange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->upHisRange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 5: _t->upEditRange(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CyMediaDisGrayStretch::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::needImage)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayStretch::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::stretchParaChange)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayStretch::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::transImageType)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayStretch::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::upStretchRange)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayStretch::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::upHisRange)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayStretch::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayStretch::upEditRange)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CyMediaDisGrayStretch::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CyMediaDisGrayStretch.data,
    qt_meta_data_CyMediaDisGrayStretch,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyMediaDisGrayStretch::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyMediaDisGrayStretch::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyMediaDisGrayStretch.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CyMediaDisGrayStretch::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void CyMediaDisGrayStretch::needImage()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CyMediaDisGrayStretch::stretchParaChange()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void CyMediaDisGrayStretch::transImageType(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CyMediaDisGrayStretch::upStretchRange(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void CyMediaDisGrayStretch::upHisRange(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CyMediaDisGrayStretch::upEditRange()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
