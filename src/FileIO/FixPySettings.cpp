#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QSettings>

#include "FixPySettings.h"
#include "Settings.h"
#include "MainWindow.h"
#include "FixPyDataProcessor.h"

FixPySettings::FixPySettings()
    : isInitialied(false), scripts()
{
}

FixPySettings::~FixPySettings()
{
    qDeleteAll(scripts);
}

QList<FixPyScript *> FixPySettings::getScripts()
{
    if (!isInitialied) {
        initialize();
    }

    return scripts;
}

FixPyScript *FixPySettings::getScript(QString name)
{
    foreach (FixPyScript *s, getScripts()) {
        if (s->name == name) {
            return s;
        }
    }

    return nullptr;
}

FixPyScript *FixPySettings::createScript(QString name)
{
    FixPyScript *script = new FixPyScript;
    script->name = name;
    FixPyDataProcessor *fixPyDp = new FixPyDataProcessor(script);
    DataProcessorFactory::instance().registerProcessor(fixPyDp);
    scripts.append(script);
    return script;
}

void FixPySettings::deleteScript(QString name)
{
    DataProcessorFactory::instance().unregisterProcessor(name);
    QList<FixPyScript *> scripts = getScripts();
    for (int i = 0; i < scripts.size(); i++) {
        FixPyScript *script = scripts[i];
        if (script->name == name) {
            this->scripts.removeAt(i);

            QSettings iniSettings(gcroot + "/" + PYFIXES_DIR_NAME + "/" + PYFIXES_SETTINGS_FILE_NAME, QSettings::IniFormat);
            iniSettings.remove(script->iniKey);
            iniSettings.sync();

            QFile pyFile(gcroot + "/" + PYFIXES_DIR_NAME + "/" + script->path);
            pyFile.remove();

            appsettings->remove(DataProcessor::configKeyAutomation(name));
            appsettings->remove(DataProcessor::configKeyAutomatedOnly(name));

            delete script;
            return;
        }
    }
}

void FixPySettings::save()
{
    QSettings iniSettings(gcroot + "/" + PYFIXES_DIR_NAME + "/" + PYFIXES_SETTINGS_FILE_NAME, QSettings::IniFormat);

    // delete old py files first and move automation keys to new name
    foreach (FixPyScript *script, getScripts()) {
        if (!script->oldPath.isEmpty()) {
            QFile oldPyFixFile(gcroot + "/" + PYFIXES_DIR_NAME + "/" + script->oldPath);
            oldPyFixFile.remove();
            script->oldPath = QString();
        }
        if (! script->oldName.isEmpty()) {
            DataProcessorFactory::instance().unregisterProcessor(script->oldName);
            QVariant automation = appsettings->value(nullptr, DataProcessor::configKeyAutomation(script->oldName), QVariant());
            QVariant automatedOnly = appsettings->value(nullptr, DataProcessor::configKeyAutomatedOnly(script->oldName), QVariant());
            if (automation.isValid()) {
                appsettings->remove(DataProcessor::configKeyAutomation(script->oldName));
                appsettings->setValue(DataProcessor::configKeyAutomation(script->name), automation);
            }
            if (automatedOnly.isValid()) {
                appsettings->remove(DataProcessor::configKeyAutomatedOnly(script->oldName));
                appsettings->setValue(DataProcessor::configKeyAutomatedOnly(script->name), automatedOnly);
            }
            FixPyDataProcessor *fixPyDp = new FixPyDataProcessor(script);
            DataProcessorFactory::instance().registerProcessor(fixPyDp);
            script->oldName = QString();
        }
    }

    int key = getMaxKey();
    foreach (FixPyScript *script, getScripts()) {
        if (script->iniKey.isEmpty()) {
            script->iniKey = QString("f%1").arg(++key);
            script->changed = true;
        }

        if (script->changed) {
            iniSettings.setValue(script->iniKey + "/name", script->name);
            iniSettings.setValue(script->iniKey + "/path", script->path);

            QFile pyFile(gcroot + "/" + PYFIXES_DIR_NAME + "/" + script->path);
            if (!pyFile.open(QIODevice::WriteOnly | QIODevice::Text)){
                continue;
            }

            QTextStream out(&pyFile);
            out << script->source;

            script->changed = false;
        }
    }

    iniSettings.sync();
}

void FixPySettings::initialize()
{
    if (isInitialied || !appsettings->value(nullptr, GC_EMBED_PYTHON, true).toBool()) {
        return;
    }

    isInitialied = true;

    QDir gcPath(gcroot);
    if (!gcPath.exists(PYFIXES_DIR_NAME)) {
        gcPath.mkpath(PYFIXES_DIR_NAME);
    }

    QSettings iniSettings(gcroot + "/" + PYFIXES_DIR_NAME + "/" + PYFIXES_SETTINGS_FILE_NAME, QSettings::IniFormat);
    iniSettings.sync();

    QDir pyfixesDir(gcroot + "/" + PYFIXES_DIR_NAME);
    QStringList pyFilters;
    pyFilters << "*.py";
    QStringList pyFixFiles = pyfixesDir.entryList(pyFilters, QDir::Files);
    foreach (QString group, iniSettings.childGroups()) {
        QString fixName = iniSettings.value(group + "/name").toString();
        QString fixPath = iniSettings.value(group + "/path").toString();
        if (fixName.isNull() || fixName.isEmpty()) {
            fixName = fixPath;
        }

        int pyFixFileIdx = pyFixFiles.indexOf(fixPath);
        if (pyFixFileIdx == -1) {
            continue;
        }

        if (!readPyFixFile(fixName, fixPath, group)) {
            pyFixFiles.removeAt(pyFixFileIdx);
            continue;
        }

        pyFixFiles.removeAt(pyFixFileIdx);
    }

    // read remaining files not listed in ini
    foreach (QString pyFixFile, pyFixFiles) {
        QString fixName(pyFixFile);
        fixName.chop(3);

        // find unique name
        for (int i = 0; i < scripts.size(); i++) {
            if (scripts[i]->name == fixName) {
                fixName = "_" + fixName;
                i = 0;
            }
        }

        readPyFixFile(fixName, pyFixFile, QString());
    }

    save();
}

void FixPySettings::disableFixPy()
{
    if (!isInitialied) return;
    isInitialied = false;

    foreach (FixPyScript *s, scripts) {
        DataProcessorFactory::instance().unregisterProcessor(s->name);
    }

    qDeleteAll(scripts);
    scripts.clear();
}

bool FixPySettings::readPyFixFile(QString fixName, QString fixPath, QString iniKey)
{
    QFile pyFile(gcroot + "/" + PYFIXES_DIR_NAME + "/" + fixPath);
    if (!pyFile.open(QIODevice::ReadOnly | QIODevice::Text)){
        return false;
    }

    QTextStream in(&pyFile);
    QString fixSource = in.readAll();

    FixPyScript *script = createScript(fixName);
    script->path = fixPath;
    script->source = fixSource;
    script->iniKey = iniKey;
    return true;
}

int FixPySettings::getMaxKey()
{
    int maxKey = -1;
    foreach (FixPyScript *fixScript, getScripts()) {
        QString iniKey = fixScript->iniKey;
        if (iniKey.length() < 2) {
            continue;
        }

        QString keyStr = iniKey.remove(0, 1);

        bool ok;
        int keyNo = keyStr.toInt(&ok);
        if (!ok) {
            continue;
        }

        if (keyNo > maxKey) {
            maxKey = keyNo;
        }
    }

    return maxKey;
}

FixPySettings *fixPySettings = new FixPySettings;
