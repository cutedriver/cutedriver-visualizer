#include "tdriver_statehistorymenu.h"

#include "tdriver_debug_macros.h"

#include <QtGui>

TDriverStateHistoryMenu::TDriverStateHistoryMenu(const QString &filePathPrefix, QWidget *parent) :
    QMenu(parent)
{
    int slashPos = filePathPrefix.lastIndexOf('/');
    parentPath = filePathPrefix.left(slashPos+1);
    QString namePrefix = filePathPrefix.mid(slashPos+1);
    dirNameFilters << namePrefix+"*";
    addAction(tr("N/A"))->setDisabled(true);
    connect(this, SIGNAL(triggered(QAction*)), SLOT(emitActivated(QAction*)));
}


void TDriverStateHistoryMenu::emitActivated(QAction *action)
{
    if (action) {
        //qDebug() << FCFL << action->data();
        emit activated(action->data().toString());
    }
}


static inline QString getDateOfFirstXmlFile(const QString &dirPath)
{
    static const QStringList filterList(QStringList() << "*.xml");
    QString ret;
    QDirIterator it(dirPath, filterList, QDir::Files);
    if (it.hasNext()) {
        it.next();
        return it.fileInfo().created().toString("yyyy-MM-dd hh:mm:ss");
    }
    else {
        ret = "N/A";
    }
    return ret;
}

void TDriverStateHistoryMenu::showEvent(QShowEvent *ev)
{
    QDirIterator it(parentPath, dirNameFilters, QDir::Dirs|QDir::NoDotAndDotDot);

    clear();

    QFileInfoList entryInfos(
                QDir(parentPath).entryInfoList(dirNameFilters,
                                               QDir::Dirs|QDir::NoDotAndDotDot,
                                               QDir::Name));

    foreach(const QFileInfo &info, entryInfos) {
        QString menuPrefix = '&' + info.fileName().right(1) + tr(" created: ");
        //qDebug() << FCFL << menuPrefix << info.absoluteFilePath();
        QString dirPath = info.canonicalFilePath();
        QAction *tmpAct = addAction(menuPrefix + getDateOfFirstXmlFile(dirPath));
        tmpAct->setData(dirPath);
        tmpAct->setStatusTip(tr("Load from directory: ") + info.canonicalFilePath());
    }

    if (isEmpty()) {
        addAction(tr("empty"))->setDisabled(true);
    }

    QMenu::showEvent(ev);
}

