#ifndef TDRIVER_STATEHISTORYMENU_H
#define TDRIVER_STATEHISTORYMENU_H

#include <QMenu>

class QShowEvent;

class TDriverStateHistoryMenu : public QMenu
{
    Q_OBJECT
public:
    explicit TDriverStateHistoryMenu(const QString &filePathPrefix, QWidget *parent = 0);

signals:
    void activated(const QString &filePath);

public slots:

private slots:
    void emitActivated(QAction *action);

protected:
    virtual void showEvent(QShowEvent *);

private:
    QString parentPath;
    QStringList dirNameFilters;
};

#endif // TDRIVER_STATEHISTORYMENU_H
