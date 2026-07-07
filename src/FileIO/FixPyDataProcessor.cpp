#include "FixPyDataProcessor.h"
#include "FixPyRunner.h"
#include "Athlete.h"

// Config widget used by the Preferences/Options config panes
class FixPyDataProcessorConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(FixPyDataProcessorConfig)

public:
    // there is no config
    FixPyDataProcessorConfig(QWidget *parent) : DataProcessorConfig(parent) {}
    void readConfig() {}
    void saveConfig() {}
};

FixPyDataProcessor::FixPyDataProcessor(FixPyScript *pyScript)
    : pyScript(pyScript)
{
}

bool FixPyDataProcessor::postProcess(RideFile *rideFile, DataProcessorConfig *settings, QString op)
{
    Q_UNUSED(settings);

    QString errText;
    bool useNewThread = op != "PYTHON";
    Context* context = (rideFile) ? rideFile->context : nullptr;
    RideItem* rideItem = nullptr;
    if (context && rideFile) {
        // get RideItem from RideFile for Python functions using it
        foreach(RideItem *item, context->athlete->rideCache->rides()) {
            if (item->dateTime == rideFile->startTime()) {
                rideItem = item;
                break;
            }
        }
    }
    FixPyRunner pyRunner(context, rideFile, rideItem, useNewThread);
    return pyRunner.run(pyScript->source, pyScript->iniKey, errText) == 0;
}

DataProcessorConfig *FixPyDataProcessor::processorConfig(QWidget *parent, const RideFile* ride) const
{
    Q_UNUSED(ride);
    return new FixPyDataProcessorConfig(parent);
}


QString
FixPyDataProcessor::explain
() const
{
    return tr("Custom Python Data Processor");
}
