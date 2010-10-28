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
    ui->setupUi(this);
    ui->outputView->setOpenLinks(false);

    updateButtonStates();

    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(process, SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
    connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(readyReadStandardOutput()));
    connect(process, SIGNAL(started()), this, SLOT(started()));
    connect(process, SIGNAL(stateChanged(QProcess::ProcessState)), this, SLOT(stateChanged(QProcess::ProcessState)));

    connect(ui->buttonBox->button(QDialogButtonBox::Abort), SIGNAL(clicked()), this, SLOT(terminate()));
    connect(ui->buttonBox->button(QDialogButtonBox::Close), SIGNAL(clicked()), this, SLOT(close()));

    connect(ui->outputView, SIGNAL(anchorClicked(QUrl)), this, SIGNAL(anchorClicked(QUrl)));
}


TDriverExecuteDialog::~TDriverExecuteDialog()
{
    //qDebug("TDriverExecuteDialog dtor");
    delete ui;
    ui = NULL;
}


void TDriverExecuteDialog::updateButtonStates()
{
    ui->buttonBox->button(QDialogButtonBox::Abort)->setEnabled(
                process->state() == QProcess::Running);

    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(
                process->state() == QProcess::NotRunning);
}


bool TDriverExecuteDialog::autoClose()
{
    return !(ui->noAutoCloseCheckbox->isChecked());
}

void TDriverExecuteDialog::terminate()
{
    if (process->state() == QProcess::Running) {
        process->terminate();
        terminateFlag = true;
        ui->outputView->setTextColor(Qt::gray);
        ui->outputView->append("<b>TERMINATE REQUESTED</b>");
    }
}


void TDriverExecuteDialog::error(QProcess::ProcessError error)
{
    // TODO: see why code commented below produces empty string
    //int enumInd = process->metaObject()->indexOfEnumerator("ProcessError");
    //QMetaEnum enumMeta = process->metaObject()->enumerator(enumInd);
    //QString enumStr(enumMeta.valueToKey(process->error());

    QString text = tr("<b>PROCESS ERROR: error code %1</b>").arg(error);
    ui->outputView->append(text);
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
    ui->outputView->setTextColor(Qt::gray);
    ui->outputView->append(text);
}


void TDriverExecuteDialog::readyReadStandardError()
{
    ui->outputView->setTextColor(Qt::darkRed);
    QString text(process->readAllStandardError());
    // for reference: C:/tdriver/visualizer/tdriver_interface.rb:33
    text.replace(QRegExp("([a-zA-Z0-9_-/:.~ ]{4,}): "), "<i><a href=\"\\1\">\\1</a></i> : ");
    ui->outputView->append(text);
}

void TDriverExecuteDialog::readyReadStandardOutput()
{
    ui->outputView->setTextColor(Qt::darkGreen);
    ui->outputView->append(process->readAllStandardOutput());
}


void TDriverExecuteDialog::started()
{
    QString argStr(cmdStr);
    if (!argList.isEmpty()) cmdStr += " " + argList.join(" ");
    QString text = tr("<b>STARTED: <i>%1</i></b>").arg(argStr);
    ui->outputView->setTextColor(Qt::gray);
    ui->outputView->append(text);
}


void TDriverExecuteDialog::stateChanged(QProcess::ProcessState /*newState*/)
{
    //ui->outputView->setTextColor(Qt::lightGray);
    //ui->outputView->append(tr("<i>PROCESS STATE changed to: %1</i>").arg(newState));
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

