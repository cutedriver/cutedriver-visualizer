#ifndef TDRIVER_EXECUTEDIALOG_H
#define TDRIVER_EXECUTEDIALOG_H

#include "libtdriverutil_global.h"

#include <QDialog>
#include <QStringList>
#include <QProcess>

class QUrl;

namespace Ui {
    class TDriverExecuteDialog;
}

class TDriverExecuteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LIBTDRIVERUTILSHARED_EXPORT TDriverExecuteDialog(
        const QString &cmd, const QStringList &args = QStringList(),
        QWidget *parent = 0, Qt::WindowFlags f = 0);

    ~TDriverExecuteDialog();

    void updateButtonStates();

signals:

    void anchorClicked(const QUrl &link);

public slots:
    void terminate();

    void error ( QProcess::ProcessError error );
    void finished ( int exitCode, QProcess::ExitStatus exitStatus );
    void readyReadStandardError ();
    void readyReadStandardOutput ();
    void started ();
    void stateChanged ( QProcess::ProcessState newState );

protected:
    virtual void showEvent(QShowEvent *);

private:
    Ui::TDriverExecuteDialog *ui;
    QProcess *process;

    QString cmdStr;
    QStringList argList;

    bool startOnShow;
    bool terminateFlag;



};

#endif // TDRIVER_EXECUTEDIALOG_H
