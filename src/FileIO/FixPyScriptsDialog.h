#ifndef FIXPYSCRIPTSDIALOG_H
#define FIXPYSCRIPTSDIALOG_H

#include <QDialog>
#include <QListWidget>

#include "GoldenCheetah.h"
#include "PythonChart.h"
#include "FixPySettings.h"

class ManageFixPyScriptsDialog : public QDialog
{
    Q_OBJECT

public:
    ManageFixPyScriptsDialog(Context *);

private slots:
    void editClicked();
    void delClicked();

private:
    void reload(int selRow);

    Context *context;
    QListWidget *scripts;
    QPushButton *edit, *del;
};

class EditFixPyScriptDialog : public QDialog, public PythonHost
{
    Q_OBJECT

public:
    EditFixPyScriptDialog(Context *, FixPyScript *fix, QWidget *parent);

    PythonChart *chart() { return nullptr; }
    bool readOnly() { return false; }

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void saveClicked();
    void runClicked();

private:
    void setScript(QString string);
    bool isModified();

    Context *context;
    FixPyScript *pyFixScript;
    FixPyRunner *pyRunner;

    PythonConsole *console;
    QLineEdit *scriptName;
    QTextEdit *script;
    QString text; // dummy if no python
};

#endif // FIXPYSCRIPTSDIALOG_H
