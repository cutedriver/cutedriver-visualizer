#ifndef TDRIVER_MAIN_TYPES_H
#define TDRIVER_MAIN_TYPES_H

#include <QString>

class QTreeWidgetItem;
template <class T> class QList;
class QRectF;

// types meant to be used in other code

typedef void* TestObjectKey;


typedef QList<QRectF> RectList;


struct TreeItemInfo {
    QString type;
    QString name;
    QString id;
    QString env;
};


struct ApplicationInfo {
    QString id;
    QString name;
    void set(const QString &id, const QString &name = QString()) { this->id=id; this->name=name; }
    bool isNull() { return id.isEmpty(); }
    void clear() { id.clear(); name.clear(); }
};


// internal convenience type, not really intended to be used outside this header

typedef QTreeWidgetItem *TestObjectPtrType;



// convenience functions meant to hide static casts

static inline TestObjectPtrType testObjectKey2Ptr(TestObjectKey key) {
    return static_cast<TestObjectPtrType>(key);
}

static inline TestObjectKey ptr2TestObjectKey(TestObjectPtrType ptr) {
    return static_cast<TestObjectKey>(ptr);
}

static inline QString testObjectKey2Str(TestObjectKey key) {
    return QString::number((quintptr)(key));
}

static inline TestObjectKey str2TestObjectKey(const QString &str) {
    return ptr2TestObjectKey(reinterpret_cast<TestObjectPtrType >(
                                 static_cast<quintptr>(str.toULongLong())));
}

#endif // TDRIVER_MAIN_TYPES_H
