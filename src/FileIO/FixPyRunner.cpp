#include <QApplication>
#include <QtConcurrent>

#include "FixPyRunner.h"
#include "PythonEmbed.h"
#include "RideFileCommand.h"

FixPyRunner::FixPyRunner(Context *context, RideFile *rideFile, bool useNewThread)
    : context(context), rideFile(rideFile), useNewThread(useNewThread)
{
}

int FixPyRunner::run(QString source, QString scriptKey, QString &errText)
{
    if (source.isEmpty()) {
        return 1;
    }

    // hourglass .. for long running ones this helps user know its busy
    QApplication::setOverrideCursor(Qt::WaitCursor);

    // set to defaults with gc applied
    python->cancelled = false;

    QString line = source;
    int result = 0;

    try {

        // replace $$ with script identifier (to avoid shared data)
        line = line.replace("$$", scriptKey);

        // run it
        FixPyRunParams params;
        params.context = context;
        params.rideFile = rideFile;
        params.script = QString(line);

        if (useNewThread) {
            QFutureWatcher<void> watcher;
            QFuture<void> f = QtConcurrent::run(execScript, &params);

            // wait for it to finish -- remember ESC can be pressed to cancel
            watcher.setFuture(f);
            QEventLoop loop;
            connect(&watcher, SIGNAL(finished()), &loop, SLOT(quit()));
            loop.exec();
        } else {
            execScript(&params);
        }

        // output on console
        if (python->messages.count()) {
            errText = python->messages.join("\n");
        }

    } catch(std::exception& ex) {
        errText = QString("\n%1\n%2").arg(QString(ex.what())).arg(python->messages.join(""));
        result = 2;
    } catch(...) {
        errText = QString("\nerror: general exception.\n%1").arg(python->messages.join(""));
        result = 3;
    }

    python->messages.clear();

    // reset cursor
    QApplication::restoreOverrideCursor();

    return result;
}

void FixPyRunner::execScript(FixPyRunParams *params)
{
    QList<RideFile *> editedRideFiles;
    python->canvas = NULL;
    python->chart = NULL;
    python->runline(ScriptContext(params->context, params->rideFile, false,
                                  false, &editedRideFiles), params->script);

    // finish up commands on edited rides
    foreach (RideFile *f, editedRideFiles) {
        f->command->endLUW();
    }
}
