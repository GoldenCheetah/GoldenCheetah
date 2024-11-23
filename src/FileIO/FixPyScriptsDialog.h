#ifndef FIXPYSCRIPTSDIALOG_H
#define FIXPYSCRIPTSDIALOG_H

#include <QDialog>
#include <QListWidget>

#include "GoldenCheetah.h"
#include "PythonChart.h"
#include "FixPySettings.h"


class ManageFixPyScriptsWidget : public QGroupBox
{
    Q_OBJECT

public:
    ManageFixPyScriptsWidget(Context *context, QWidget *parent = nullptr);

private slots:
    void newClicked();
    void editClicked();
    void delClicked();
    void scriptSelected();

private:
    void reload();
    void reload(int selRow);
    void reload(const QString &selName);

    Context *context;
    QListWidget *scripts;
    QPushButton *edit, *del;
};


class ManageFixPyScriptsDialog : public QDialog
{
    Q_OBJECT

public:
    ManageFixPyScriptsDialog(Context *context);
};


class EditFixPyScriptDialog : public QDialog, public PythonHost
{
    Q_OBJECT

public:
    EditFixPyScriptDialog(Context *, FixPyScript *fix, QWidget *parent);

    PythonChart *chart() { return nullptr; }
    bool readOnly() { return false; }
    QString getName() const;

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
