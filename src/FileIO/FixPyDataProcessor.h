#ifndef FIXPYDATAPROCESSOR_H
#define FIXPYDATAPROCESSOR_H

#include "DataProcessor.h"
#include "FixPyScript.h"

class FixPyDataProcessor : public DataProcessor
{
public:
    FixPyDataProcessor(FixPyScript *pyScript);
    bool postProcess(RideFile *rideFile, DataProcessorConfig *settings, QString op);
    DataProcessorConfig *processorConfig(QWidget *parent, const RideFile* ride = NULL);
    QString name() { return pyScript->name; }
    bool isCoreProcessor() { return false; }

private:
    FixPyScript *pyScript;
};

#endif // FIXPYDATAPROCESSOR_H
