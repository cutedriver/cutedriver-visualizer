#ifndef TDRIVER_PROCESSDIALOG_H
#define TDRIVER_PROCESSDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QProcess>

class QPlainTextEdit;

class TDriverProcessDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TDriverProcessDialog(const QString &cmd, const QStringList &args = QStringList(),
                                  QWidget *parent = 0, Qt::WindowFlags f = 0);


signals:

public slots:
    void	error ( QProcess::ProcessError error );
    void	finished ( int exitCode, QProcess::ExitStatus exitStatus );
    void	readyReadStandardError ();
    void	readyReadStandardOutput ();
    void	started ();
    void	stateChanged ( QProcess::ProcessState newState );

protected:
    virtual void showEvent(QShowEvent *);

private:
    QProcess *process;
    QPlainTextEdit *view;

    QString cmdStr;
    QStringList argList;

    bool startOnShow;
};

#endif // TDRIVER_PROCESSDIALOG_H
