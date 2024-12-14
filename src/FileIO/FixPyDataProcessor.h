#ifndef FIXPYDATAPROCESSOR_H
#define FIXPYDATAPROCESSOR_H

#include "DataProcessor.h"
#include "FixPyScript.h"

class FixPyDataProcessor : public DataProcessor
{
    Q_DECLARE_TR_FUNCTIONS(FixPyDataProcessor)

public:
    FixPyDataProcessor(FixPyScript *pyScript);
    bool postProcess(RideFile *rideFile, DataProcessorConfig *settings, QString op) override;
    DataProcessorConfig *processorConfig(QWidget *parent, const RideFile* ride = NULL) const override;
    QString name() const override { return pyScript->name; }
    QString id() const override { return pyScript->name; }
    bool isCoreProcessor() const override { return false; }
    QString explain() const override;

private:
    FixPyScript *pyScript;
};

#endif // FIXPYDATAPROCESSOR_H
