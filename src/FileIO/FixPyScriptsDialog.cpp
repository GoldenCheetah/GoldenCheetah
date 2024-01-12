#include <QGroupBox>

#include "FixPyScriptsDialog.h"
#include "MainWindow.h"
#include "Colors.h"
#include "AbstractView.h"
#include "PythonEmbed.h"
#include "PythonSyntax.h"

ManageFixPyScriptsDialog::ManageFixPyScriptsDialog(Context *context)
    : context(context)
{
    setWindowTitle(tr("Manage Python Fixes"));
    setMinimumSize(QSize(700 * dpiXFactor, 600 * dpiYFactor));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QGroupBox *groupBox = new QGroupBox(tr("Select a Python Fix to manage"));
    QHBoxLayout *manageBox = new QHBoxLayout();

    scripts = new QListWidget;
    manageBox->addWidget(scripts);

    QVBoxLayout *manageButtons = new QVBoxLayout();
    edit = new QPushButton(tr("Edit"));
    del = new QPushButton(tr("Delete"));
    manageButtons->addWidget(edit);
    manageButtons->addWidget(del);
    manageButtons->addStretch();
    manageBox->addLayout(manageButtons);

    groupBox->setLayout(manageBox);
    mainLayout->addWidget(groupBox);

    QPushButton *close = new QPushButton(tr("Close"));
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(close);

    mainLayout->addLayout(buttons);

    reload(0);

    connect(edit, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(del, SIGNAL(clicked()), this, SLOT(delClicked()));
    connect(close, SIGNAL(clicked()), this, SLOT(reject()));
}

void ManageFixPyScriptsDialog::editClicked()
{
    QList<QListWidgetItem*> selItems = scripts->selectedItems();
    if (selItems.length() == 0) {
        return;
    }

    QListWidgetItem *selItem = selItems[0];
    int selRow = scripts->row(selItem);
    FixPyScript *script = fixPySettings->getScript(selItem->text());

    EditFixPyScriptDialog editDlg(context, script, this);
    if (editDlg.exec() == QDialog::Accepted) {
        reload(selRow);
    }
}

void ManageFixPyScriptsDialog::delClicked()
{
    QList<QListWidgetItem*> selItems = scripts->selectedItems();
    if (selItems.length() == 0) {
        return;
    }

    QListWidgetItem *selItem = selItems[0];
    QString name = selItem->text();

    QString msg = QString(tr("Are you sure you want to delete %1?")).arg(name);
    QMessageBox::StandardButtons result = QMessageBox::question(this, "GoldenCheetah", msg);
    if (result == QMessageBox::No) {
        return;
    }

    fixPySettings->deleteScript(name);

    reload(0);
}

void ManageFixPyScriptsDialog::reload(int selRow)
{
    scripts->clear();
    QList<FixPyScript *> fixes = fixPySettings->getScripts();
    for (int i = 0; i < fixes.size(); i++) {
        scripts->addItem(fixes[i]->name);
    }

    if (scripts->count() > 0) {
        scripts->setCurrentRow(selRow);
    } else {
        edit->setEnabled(false);
        del->setEnabled(false);
    }
}

EditFixPyScriptDialog::EditFixPyScriptDialog(Context *context, FixPyScript *fix, QWidget *parent)
    : QDialog(parent), context(context), pyFixScript(fix)
{
    setWindowTitle(tr("Edit Python Fix"));
    setMinimumSize(QSize(1200 * dpiXFactor, 650 * dpiYFactor));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QSplitter *outerSplitter = new QSplitter(Qt::Horizontal);
    outerSplitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    outerSplitter->setHandleWidth(1);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->setHandleWidth(1);

    // LHS
    QFrame *scriptBox = new QFrame;
    scriptBox->setFrameStyle(QFrame::NoFrame);
    QVBoxLayout *scriptLayout = new QVBoxLayout;
    scriptLayout->setContentsMargins(0, 0, 0, 10);
    scriptBox->setLayout(scriptLayout);

    QHBoxLayout *nameLayout = new QHBoxLayout;
    QLabel *scriptNameLbl = new QLabel(tr("Name:"));
    scriptName = new QLineEdit(fix ? fix->name : "");
    nameLayout->addWidget(scriptNameLbl);
    nameLayout->addWidget(scriptName);
    scriptLayout->addLayout(nameLayout);

    script = new QTextEdit;
    script->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    script->setFrameStyle(QFrame::NoFrame);
    script->setAcceptRichText(false);
    QFont courier("Courier", QFont().pointSize());
    script->setFont(courier);
    QPalette p = palette();
    p.setColor(QPalette::Base, GColor(CPLOTBACKGROUND));
    p.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
    script->setPalette(p);
    script->setStyleSheet(AbstractView::ourStyleSheet());
    scriptLayout->addWidget(script);

    // syntax highlighter
    setScript(fix ? fix->source : "");

    QPushButton *run = new QPushButton(tr("Run"));
    scriptLayout->addWidget(run);

    splitter->addWidget(scriptBox);
    console = new PythonConsole(context, this, this);
    console->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->addWidget(console);

    outerSplitter->addWidget(splitter);

    // ride editor
    GcChartWindow *win = GcWindowRegistry::newGcWindow(GcWindowTypes::MetadataWindow, context);
    if (win) {
        win->setProperty("nomenu", true);

        RideItem *notconst = (RideItem*)context->currentRideItem();
        win->setProperty("ride", QVariant::fromValue<RideItem*>(notconst));
        DateRange dr = context->currentDateRange();
        win->setProperty("dateRange", QVariant::fromValue<DateRange>(dr));
        outerSplitter->addWidget(win);
    }

    mainLayout->addWidget(outerSplitter);

    QPushButton *save = new QPushButton(tr("Save and close"));
    QPushButton *cancel = new QPushButton(tr("Cancel"));
    QHBoxLayout *buttons = new QHBoxLayout();
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(save);

    mainLayout->addLayout(buttons);

    connect(run, SIGNAL(clicked()), this, SLOT(runClicked()));
    connect(save, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
}

void EditFixPyScriptDialog::closeEvent(QCloseEvent *event)
{
    if (!isModified()) {
        event->accept();
        return;
    }

    QMessageBox msgBox;
    msgBox.setText(tr("The Python Fix has been modified."));
    msgBox.setInformativeText(tr("Do you want to save your changes?"));
    msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Save);
    int result = msgBox.exec();

    switch (result) {
    case QMessageBox::Save:
        event->ignore();
        saveClicked();
        break;
    case QMessageBox::Discard:
        event->accept();
        break;
    case QMessageBox::Cancel:
        event->ignore();
        break;
    }
}

void EditFixPyScriptDialog::setScript(QString string)
{
    if (python && script) {
        script->setText(string);
        new PythonSyntax(script->document());
    }

    text = string;
}

bool EditFixPyScriptDialog::isModified()
{
    return scriptName->isModified() ||
           script->document()->isModified();
}

void EditFixPyScriptDialog::saveClicked()
{
    QString name = scriptName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::critical(this, "GoldenCheetah", tr("Please specify a name for the Python Fix."));
        return;
    }

    QString illegal = "<>:\"|?*";
    foreach (const QChar& c, name)
    {
        // Check for illegal characters
        if (illegal.contains(c)) {
            QMessageBox::critical(this, "GoldenCheetah",
                                  QString(tr("The Python Fix name may not contain any of the following characters: %1")).arg(illegal));
            return;
        }
    }

    QList<FixPyScript *> fixScripts = fixPySettings->getScripts();
    foreach (FixPyScript *fixScript, fixScripts) {
        if (fixScript == pyFixScript) {
            continue;
        }

        if (name == fixScript->name) {
            QMessageBox::critical(this, "GoldenCheetah",
                                  tr("A Python Fix with that name exists already. Please choose another name."));
            return;
        }
    }

    // ensure path is unique
    QString path(name);
    path = path.replace(" ", "_").toLower() + ".py";
    for (int i = 0; i < fixScripts.size(); i++) {
        FixPyScript *fixScript = fixScripts[i];
        if (fixScript == pyFixScript) {
            continue;
        }

        if (path == fixScript->path) {
            path = "_" + path;
            i = 0;
        }
    }

    if (pyFixScript == nullptr) {
        pyFixScript = fixPySettings->createScript(name);
    }

    // remember old path, for clean up
    if (!pyFixScript->path.isEmpty() && pyFixScript->path != path) {
        pyFixScript->oldPath = pyFixScript->path;
    }

    pyFixScript->name = name;
    pyFixScript->source = script->toPlainText();
    pyFixScript->path = path;
    pyFixScript->changed = true;

    fixPySettings->save();
    accept();
}

void EditFixPyScriptDialog::runClicked()
{
    QString key = QUuid::createUuid().toString();
    key = key.replace("-", "_");

    setUpdatesEnabled(false);

    QString errText;
    FixPyRunner pyRunner(context);
    pyRunner.run(script->toPlainText(), "_" + key, errText);
    if (!errText.isEmpty()) {
        console->putData(errText);
    }

    setUpdatesEnabled(true);
}
