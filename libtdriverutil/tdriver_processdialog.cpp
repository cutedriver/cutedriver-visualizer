#include "tdriver_ExecuteDialog.h"

#include <QPlainTextEdit>

TDriverExecuteDialog::TDriverExecuteDialog(const QString &cmd, const QStringList &args,
                                           QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    process(new QProcess(this)),
    view(new QPlainTextEdit()),
    cmdStr(cmd),
    argList(args),
    startOnShow(true)
{
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(process, SIGNAL(finished(int)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
    connect(process, SIGNAL(started()), this, SLOT(started()));
    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

    Q
}





void TDriverExecuteDialog::error(QProcess::ProcessError error)
{
}

void TDriverExecuteDialog::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
}

void TDriverExecuteDialog::readyReadStandardError()
{
}

void TDriverExecuteDialog::readyReadStandardOutput()
{
}

void TDriverExecuteDialog::started()
{
}

void TDriverExecuteDialog::stateChanged(QProcess::ProcessState newState)
{
}

void TDriverExecuteDialog::showEvent(QShowEvent *)
{
    if (startOnShow && process->state() == QProcess::NotRunning) {
        process->start(cmdStr, argList);
        startOnShow = false;
    }
}




