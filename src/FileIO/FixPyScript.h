#ifndef FIXPYSCRIPT_H
#define FIXPYSCRIPT_H

#include <QString>

struct FixPyScript {
    QString name, path, source, iniKey, oldPath;
    bool automatedOnly = false;
    bool changed;
};

#endif // FIXPYSCRIPT_H
