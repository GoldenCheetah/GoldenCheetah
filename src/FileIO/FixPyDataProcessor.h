#ifndef FIXPYDATAPROCESSOR_H
#define FIXPYDATAPROCESSOR_H

#include "DataProcessor.h"
#include "FixPyScript.h"

class FixPyDataProcessor : public DataProcessor
{
public:
    FixPyDataProcessor(FixPyScript *pyScript);
    bool postProcess(RideFile *rideFile, DataProcessorConfig *settings, QString op) override;
    DataProcessorConfig *processorConfig(QWidget *parent, const RideFile* ride = NULL) override;
    QString name() override { return pyScript->name; }
    bool isCoreProcessor() const override { return false; }
    bool isAutomatedOnly() const override;

private:
    FixPyScript *pyScript;
};

#endif // FIXPYDATAPROCESSOR_H
