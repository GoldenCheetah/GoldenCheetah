#ifndef FIXPYSETTINGS_H
#define FIXPYSETTINGS_H

#include <QString>
#include <QList>

#include "FixPyScript.h"
#include "FixPyRunner.h"

class FixPySettings
{
public:
    FixPySettings();
    ~FixPySettings();

    void initialize();
    void disableFixPy();
    QList<FixPyScript *> getScripts();
    FixPyScript *getScript(QString name);
    FixPyScript *createScript(QString name);
    void deleteScript(QString name);

    void save();

private:
    const QString PYFIXES_DIR_NAME = ".pyfixes";
    const QString PYFIXES_SETTINGS_FILE_NAME = "configglobal-pyfixes.ini";

    bool readPyFixFile(QString fixName, QString fixPath, QString iniKey);
    int getMaxKey();

    bool isInitialied;
    QList<FixPyScript *> scripts;
};

extern FixPySettings *fixPySettings;

#endif // FIXPYSETTINGS_H
