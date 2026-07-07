#ifndef FIXPYSCRIPT_H
#define FIXPYSCRIPT_H

#include <QString>

struct FixPyScript {
    QString name, path, source, iniKey;
    QString oldName, oldPath;
    bool changed;
};

#endif // FIXPYSCRIPT_H
