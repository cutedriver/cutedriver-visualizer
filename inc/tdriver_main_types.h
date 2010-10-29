#ifndef TDRIVER_MAIN_TYPES_H
#define TDRIVER_MAIN_TYPES_H

#include <QString>

typedef void* ProcessKey;
typedef void* TestObjectKey;

static inline QString testObjectKey2Str(TestObjectKey key) { return QString::number((ulong)key); }

#endif // TDRIVER_MAIN_TYPES_H
