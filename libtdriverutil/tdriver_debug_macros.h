#ifndef TDRIVER_DEBUG_MACROS_H
#define TDRIVER_DEBUG_MACROS_H

#include <QDebug>
#define FFL (QString("%1/%2:%3:").arg(__FILE__).arg(__FUNCTION__ ).arg(__LINE__ ).toAscii().constData())
#define FCFL (QString("%1/%2::%3:%4:").arg(__FILE__).arg(metaObject()->className()).arg(__FUNCTION__ ).arg(__LINE__ ).toAscii().constData())

#include <QTime>

#endif // TDRIVER_DEBUG_MACROS_H
