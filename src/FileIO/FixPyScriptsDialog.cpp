#include <QGroupBox>

#include "FixPyScriptsDialog.h"
#include "MainWindow.h"
#include "Colors.h"
#include "AbstractView.h"
#include "PythonEmbed.h"
#include "PythonSyntax.h"


ManageFixPyScriptsWidget::ManageFixPyScriptsWidget
(Context *context, QWidget *parent)
: QGroupBox(tr("Select a Python Fix to manage"), parent), context(context)
{
    scripts = new QListWidget;
    scripts->setAlternatingRowColors(true);

    QDialogButtonBox *manageButtonBox = new QDialogButtonBox();
    manageButtonBox->setOrientation(Qt::Vertical);
    QPushButton *newButton = manageButtonBox->addButton(tr("New"), QDialogButtonBox::ActionRole);
    edit = manageButtonBox->addButton(tr("Edit"), QDialogButtonBox::ActionRole);
    del = manageButtonBox->addButton(tr("Delete"), QDialogButtonBox::ActionRole);

    QHBoxLayout *manageBox = new QHBoxLayout(this);
    manageBox->addWidget(scripts);
    manageBox->addWidget(manageButtonBox);

    connect(newButton, &QPushButton::clicked, this, &ManageFixPyScriptsWidget::newClicked);
    connect(scripts, &QListWidget::itemDoubleClicked, this, &ManageFixPyScriptsWidget::editClicked);
    connect(scripts, &QListWidget::itemSelectionChanged, this, &ManageFixPyScriptsWidget::scriptSelected);
    connect(edit, &QPushButton::clicked, this, &ManageFixPyScriptsWidget::editClicked);
    connect(del, &QPushButton::clicked, this, &ManageFixPyScriptsWidget::delClicked);

    reload(0);
}


void
ManageFixPyScriptsWidget::newClicked
()
{
    EditFixPyScriptDialog editDlg(context, nullptr, this);
    if (editDlg.exec() == QDialog::Accepted) {
        reload(editDlg.getName());
    }
}


void
ManageFixPyScriptsWidget::editClicked
()
{
    if (scripts->currentItem() == nullptr) {
        return;
    }
    FixPyScript *script = fixPySettings->getScript(scripts->currentItem()->text());

    EditFixPyScriptDialog editDlg(context, script, this);
    if (editDlg.exec() == QDialog::Accepted) {
        reload(editDlg.getName());
    }
}


void
ManageFixPyScriptsWidget::delClicked
()
{
    if (scripts->currentItem() == nullptr) {
        return;
    }
    QString name = scripts->currentItem()->text();

    QString msg = QString(tr("Are you sure you want to delete %1?")).arg(name);
    QMessageBox::StandardButtons result = QMessageBox::question(this, "GoldenCheetah", msg);
    if (result == QMessageBox::No) {
        return;
    }

    fixPySettings->deleteScript(name);

    reload(scripts->currentRow());
}


void
ManageFixPyScriptsWidget::scriptSelected
()
{
    edit->setEnabled(scripts->currentItem() != nullptr);
    del->setEnabled(scripts->currentItem() != nullptr);
}


void
ManageFixPyScriptsWidget::reload
()
{
    scripts->clear();
    QList<FixPyScript*> fixes = fixPySettings->getScripts();
    for (int i = 0; i < fixes.size(); i++) {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(fixes[i]->name);
        scripts->addItem(item);
    }
    edit->setEnabled(false);
    del->setEnabled(false);
}


void
ManageFixPyScriptsWidget::reload
(int selRow)
{
    reload();
    QList<FixPyScript*> fixes = fixPySettings->getScripts();
    if (scripts->count() > 0) {
        scripts->setCurrentRow(std::max(0, std::min(selRow, scripts->count() - 1)));
    }
}


void
ManageFixPyScriptsWidget::reload
(const QString &selName)
{
    reload();
    QList<FixPyScript*> fixes = fixPySettings->getScripts();
    for (int i = 0; i < fixes.size(); i++) {
        if (fixes[i]->name == selName) {
            scripts->setCurrentRow(i);
            return;
        }
    }
}



ManageFixPyScriptsDialog::ManageFixPyScriptsDialog
(Context *context)
{
    setWindowTitle(tr("Manage Python Fixes"));
    setMinimumSize(QSize(700 * dpiXFactor, 600 * dpiYFactor));

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new ManageFixPyScriptsWidget(context));
    mainLayout->addWidget(buttonBox);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
    QFormLayout *scriptLayout = newQFormLayout();
    scriptLayout->setContentsMargins(0, 0, 0, 10);
    scriptBox->setLayout(scriptLayout);

    scriptName = new QLineEdit(fix ? fix->name : "");

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

    // syntax highlighter
    setScript(fix ? fix->source : "");

    QPushButton *run = new QPushButton(tr("Run"));

    scriptLayout->addRow(tr("Name"), scriptName);
    scriptLayout->addRow(script);
    scriptLayout->addRow(run);

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


QString
EditFixPyScriptDialog::getName
() const
{
    if (pyFixScript != nullptr) {
        return pyFixScript->name;
    } else {
        return QString();
    }
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
    return    scriptName->isModified()
           || script->document()->isModified();
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

    // remember old name, for clean up
    if (!pyFixScript->name.isEmpty() && pyFixScript->name != name) {
        pyFixScript->oldName = pyFixScript->name;
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
