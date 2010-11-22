#ifndef TDRIVER_DEBUG_MACROS_H
#define TDRIVER_DEBUG_MACROS_H

#include <QDebug>
#include <QString>
#include <QStringList>

static inline QString prettyFile(const char *file) {
    QStringList temp = QString(file).replace('\\', '/').split('/');
    if (temp.length() > 3) temp = temp.mid(temp.length()-3);
    return temp.join("/");
}

//#define FFL (QString("%1/%2:%3:").arg(prettyFile(__FILE__)).arg(__FUNCTION__ ).arg(__LINE__ ).toLocal8Bit().constData())
#define FFL (QString("%1:%3 (%2):").arg(prettyFile(__FILE__)).arg(__FUNCTION__ ).arg(__LINE__ ).toLocal8Bit().constData())
#define FCFL (QString("%1:%4 (%2::%3):").arg(prettyFile(__FILE__)).arg(metaObject()->className()).arg(__FUNCTION__ ).arg(__LINE__ ).toLocal8Bit().constData())
//#define FCFL ((QString("%1:%2:").arg(__PRETTY_FUNCTION__).arg(__LINE__ )).toLocal8Bit().constData())

#include <QTime>

#endif // TDRIVER_DEBUG_MACROS_H
