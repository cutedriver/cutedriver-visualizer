#include "tdriver_executedialog.h"
#include "ui_tdriver_executedialog.h"

#include <QPushButton>
#include <QMetaEnum>

TDriverExecuteDialog::TDriverExecuteDialog(const QString &cmd, const QStringList &args,
                                           QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent, f),
    ui(new Ui::TDriverExecuteDialog),
    process(new QProcess(this)),
    cmdStr(cmd),
    argList(args),
    startOnShow(true),
    terminateFlag(false)
{
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->setupUi(this);
    ui->outputView->setReadOnly(true);
    updateButtonStates();

    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
    connect(process, SIGNAL(started()), this, SLOT(started()));
    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

    connect(ui->buttonBox->button(QDialogButtonBox::Abort), SIGNAL(clicked()), this, SLOT(terminate()));
    connect(ui->buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(close()));
}


TDriverExecuteDialog::~TDriverExecuteDialog()
{
    delete ui;
    ui = NULL;
}


void TDriverExecuteDialog::updateButtonStates()
{
    ui->buttonBox->button(QDialogButtonBox::Abort)->setEnabled(
                process->state() == QProcess::Running);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                process->state() == QProcess::NotRunning);
}


void TDriverExecuteDialog::terminate()
{
    if (process->state() == QProcess::Running) {
        process->terminate();
        terminateFlag = true;
        ui->outputView->appendHtml("<b>TERMINATE REQUESTED</b>");
    }
}


void TDriverExecuteDialog::error(QProcess::ProcessError error)
{
    // TODO: see why code commented below produces empty string
    //int enumInd = process->metaObject()->indexOfEnumerator("ProcessError");
    //QMetaEnum enumMeta = process->metaObject()->enumerator(enumInd);
    //QString enumStr(enumMeta.valueToKey(process->error());

    QString text = tr("<b>PROCESS ERROR: error code %1</b>")
            .arg(process->error());

    ui->outputView->appendHtml(text);
}


void TDriverExecuteDialog::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString text;
    if (exitStatus == QProcess::NormalExit) {
        text = tr("<b>NORMAL EXIT: exit code %1</b>").arg(exitCode);
    }
    else {
        text = tr("<b>ABNORMAL EXIT</b>");
    }
    ui->outputView->appendHtml(text);
}


void TDriverExecuteDialog::readyReadStandardError()
{
    ui->outputView->appendPlainText(process->readAllStandardError());
}

void TDriverExecuteDialog::readyReadStandardOutput()
{
    ui->outputView->appendPlainText(process->readAllStandardOutput());
}


void TDriverExecuteDialog::started()
{
    QString argStr(cmdStr);
    if (!argList.isEmpty()) cmdStr += " " + argList.join(" ");
    QString text = tr("<b>STARTED: <i>%1</i></b>").arg(argStr);
    ui->outputView->appendHtml(text);
}


void TDriverExecuteDialog::stateChanged(QProcess::ProcessState newState)
{
    //ui->outputView->appendHtml(tr("<i>PROCESS STATE changed to: %1</i>").arg(newState));
    updateButtonStates();
}


void TDriverExecuteDialog::showEvent(QShowEvent *)
{
    if (startOnShow && process->state() == QProcess::NotRunning) {
        process->start(cmdStr, argList);
        startOnShow = false;
    }
    updateButtonStates();
}
