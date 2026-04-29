/****************************************************************************
** Meta object code from reading C++ file 'cycustomwidget.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.14.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../../../CyMedia/CyMediaDis/cycustomwidget.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'cycustomwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.14.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_CyCustomWidget__CyRangeSlider_t {
    QByteArrayData data[8];
    char stringdata0[116];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyCustomWidget__CyRangeSlider_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyCustomWidget__CyRangeSlider_t qt_meta_stringdata_CyCustomWidget__CyRangeSlider = {
    {
QT_MOC_LITERAL(0, 0, 29), // "CyCustomWidget::CyRangeSlider"
QT_MOC_LITERAL(1, 30, 17), // "firstValueChanged"
QT_MOC_LITERAL(2, 48, 0), // ""
QT_MOC_LITERAL(3, 49, 8), // "firstNum"
QT_MOC_LITERAL(4, 58, 18), // "secondValueCHanged"
QT_MOC_LITERAL(5, 77, 9), // "secondNum"
QT_MOC_LITERAL(6, 87, 15), // "allValueChanged"
QT_MOC_LITERAL(7, 103, 12) // "mouseRelease"

    },
    "CyCustomWidget::CyRangeSlider\0"
    "firstValueChanged\0\0firstNum\0"
    "secondValueCHanged\0secondNum\0"
    "allValueChanged\0mouseRelease"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyCustomWidget__CyRangeSlider[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   34,    2, 0x06 /* Public */,
       4,    1,   37,    2, 0x06 /* Public */,
       6,    2,   40,    2, 0x06 /* Public */,
       7,    0,   45,    2, 0x06 /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::Int,    3,
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    3,    5,
    QMetaType::Void,

       0        // eod
};

void CyCustomWidget::CyRangeSlider::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CyRangeSlider *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->firstValueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->secondValueCHanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->allValueChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 3: _t->mouseRelease(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CyRangeSlider::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyRangeSlider::firstValueChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (CyRangeSlider::*)(int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyRangeSlider::secondValueCHanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (CyRangeSlider::*)(int , int );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyRangeSlider::allValueChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (CyRangeSlider::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyRangeSlider::mouseRelease)) {
                *result = 3;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CyCustomWidget::CyRangeSlider::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CyCustomWidget__CyRangeSlider.data,
    qt_meta_data_CyCustomWidget__CyRangeSlider,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyCustomWidget::CyRangeSlider::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyCustomWidget::CyRangeSlider::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyCustomWidget__CyRangeSlider.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CyCustomWidget::CyRangeSlider::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 4;
    }
    return _id;
}

// SIGNAL 0
void CyCustomWidget::CyRangeSlider::firstValueChanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void CyCustomWidget::CyRangeSlider::secondValueCHanged(int _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void CyCustomWidget::CyRangeSlider::allValueChanged(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void CyCustomWidget::CyRangeSlider::mouseRelease()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
struct qt_meta_stringdata_CyCustomWidget__CyCheckButton_t {
    QByteArrayData data[7];
    char stringdata0[80];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyCustomWidget__CyCheckButton_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyCustomWidget__CyCheckButton_t qt_meta_stringdata_CyCustomWidget__CyCheckButton = {
    {
QT_MOC_LITERAL(0, 0, 29), // "CyCustomWidget::CyCheckButton"
QT_MOC_LITERAL(1, 30, 12), // "stateChanged"
QT_MOC_LITERAL(2, 43, 0), // ""
QT_MOC_LITERAL(3, 44, 7), // "checked"
QT_MOC_LITERAL(4, 52, 10), // "setChecked"
QT_MOC_LITERAL(5, 63, 5), // "check"
QT_MOC_LITERAL(6, 69, 10) // "needSignal"

    },
    "CyCustomWidget::CyCheckButton\0"
    "stateChanged\0\0checked\0setChecked\0check\0"
    "needSignal"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyCustomWidget__CyCheckButton[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   29,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       4,    2,   32,    2, 0x0a /* Public */,
       4,    1,   37,    2, 0x2a /* Public | MethodCloned */,

 // signals: parameters
    QMetaType::Void, QMetaType::Bool,    3,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::Bool,    5,    6,
    QMetaType::Void, QMetaType::Bool,    5,

       0        // eod
};

void CyCustomWidget::CyCheckButton::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<CyCheckButton *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 1: _t->setChecked((*reinterpret_cast< bool(*)>(_a[1])),(*reinterpret_cast< bool(*)>(_a[2]))); break;
        case 2: _t->setChecked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (CyCheckButton::*)(bool );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&CyCheckButton::stateChanged)) {
                *result = 0;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject CyCustomWidget::CyCheckButton::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CyCustomWidget__CyCheckButton.data,
    qt_meta_data_CyCustomWidget__CyCheckButton,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyCustomWidget::CyCheckButton::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyCustomWidget::CyCheckButton::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyCustomWidget__CyCheckButton.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int CyCustomWidget::CyCheckButton::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void CyCustomWidget::CyCheckButton::stateChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
struct qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog_t {
    QByteArrayData data[1];
    char stringdata0[35];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog_t qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog = {
    {
QT_MOC_LITERAL(0, 0, 34) // "CyCustomWidget::SaveFileDoneD..."

    },
    "CyCustomWidget::SaveFileDoneDialog"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyCustomWidget__SaveFileDoneDialog[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void CyCustomWidget::SaveFileDoneDialog::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject CyCustomWidget::SaveFileDoneDialog::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog.data,
    qt_meta_data_CyCustomWidget__SaveFileDoneDialog,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyCustomWidget::SaveFileDoneDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyCustomWidget::SaveFileDoneDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyCustomWidget__SaveFileDoneDialog.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CyCustomWidget::SaveFileDoneDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    return _id;
}
struct qt_meta_stringdata_CyCustomWidget__waitWidget_t {
    QByteArrayData data[1];
    char stringdata0[27];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_CyCustomWidget__waitWidget_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_CyCustomWidget__waitWidget_t qt_meta_stringdata_CyCustomWidget__waitWidget = {
    {
QT_MOC_LITERAL(0, 0, 26) // "CyCustomWidget::waitWidget"

    },
    "CyCustomWidget::waitWidget"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_CyCustomWidget__waitWidget[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

void CyCustomWidget::waitWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    Q_UNUSED(_o);
    Q_UNUSED(_id);
    Q_UNUSED(_c);
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject CyCustomWidget::waitWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QDialog::staticMetaObject>(),
    qt_meta_stringdata_CyCustomWidget__waitWidget.data,
    qt_meta_data_CyCustomWidget__waitWidget,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *CyCustomWidget::waitWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *CyCustomWidget::waitWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CyCustomWidget__waitWidget.stringdata0))
        return static_cast<void*>(this);
    return QDialog::qt_metacast(_clname);
}

int CyCustomWidget::waitWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
