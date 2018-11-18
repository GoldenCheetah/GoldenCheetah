#ifndef FIXPYRUNNER_H
#define FIXPYRUNNER_H

#include <QObject>
#include <QString>

#include "FixPyScript.h"
#include "Context.h"

struct FixPyRunParams
{
    Context *context;
    RideFile *rideFile;
    QString script;
};

class FixPyRunner : public QObject
{
    Q_OBJECT

public:
    FixPyRunner(Context *context = nullptr, RideFile *rideFile = nullptr, bool useNewThread = true);

    int run(QString source, QString scriptKey, QString &errText);
    static void execScript(FixPyRunParams *params);

private:
    Context *context;
    RideFile *rideFile;
    bool useNewThread;
};

#endif // FIXPYRUNNER_H
