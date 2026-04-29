/****************************************************************************
** Meta object code from reading C++ file 'CyMediaDisGrayTest.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../CyMedia/CyMediaDis/CyMediaDisGrayTest.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'CyMediaDisGrayTest.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CyMediaDisGrayTest_t {
    QByteArrayData data[12];
    char stringdata0[113];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyMediaDisGrayTest_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyMediaDisGrayTest_t qt_meta_stringdata_CyMediaDisGrayTest = {
    {
QT_MOC_LITERAL(0, 0, 18), // "CyMediaDisGrayTest"
QT_MOC_LITERAL(1, 19, 9), // "needImage"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 14), // "testModeChange"
QT_MOC_LITERAL(4, 45, 8), // "drawType"
QT_MOC_LITERAL(5, 54, 12), // "uphisVisible"
QT_MOC_LITERAL(6, 67, 10), // "upTestData"
QT_MOC_LITERAL(7, 78, 10), // "upHisRange"
QT_MOC_LITERAL(8, 89, 4), // "minX"
QT_MOC_LITERAL(9, 94, 4), // "maxX"
QT_MOC_LITERAL(10, 99, 4), // "maxY"
QT_MOC_LITERAL(11, 104, 8) // "flushHis"

    },
    "CyMediaDisGrayTest\0needImage\0\0"
    "testModeChange\0drawType\0uphisVisible\0"
    "upTestData\0upHisRange\0minX\0maxX\0maxY\0"
    "flushHis"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyMediaDisGrayTest[] = {

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
       3,    1,   45,    2, 0x06 /* Public */,
       5,    0,   48,    2, 0x06 /* Public */,
       6,    0,   49,    2, 0x06 /* Public */,
       7,    3,   50,    2, 0x06 /* Public */,
      11,    0,   57,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int,    4,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Int, QMetaType::Int, QMetaType::Int,    8,    9,   10,
    QMetaType::Void,

       0        // eod
};

void CyMediaDisGrayTest::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CyMediaDisGrayTest *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->needImage(); break;
        case 1: _t->testModeChange((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->uphisVisible(); break;
        case 3: _t->upTestData(); break;
        case 4: _t->upHisRange((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 5: _t->flushHis(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CyMediaDisGrayTest::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::needImage)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayTest::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::testModeChange)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayTest::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::uphisVisible)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayTest::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::upTestData)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayTest::*)(int , int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::upHisRange)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (CyMediaDisGrayTest::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyMediaDisGrayTest::flushHis)) {
                *result = 5;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CyMediaDisGrayTest::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CyMediaDisGrayTest.data,
    qt_meta_data_CyMediaDisGrayTest,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyMediaDisGrayTest::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyMediaDisGrayTest::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyMediaDisGrayTest.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CyMediaDisGrayTest::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void CyMediaDisGrayTest::needImage()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void CyMediaDisGrayTest::testModeChange(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CyMediaDisGrayTest::uphisVisible()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void CyMediaDisGrayTest::upTestData()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void CyMediaDisGrayTest::upHisRange(int _t1, int _t2, int _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void CyMediaDisGrayTest::flushHis()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
