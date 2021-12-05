#include "FixPyDataProcessor.h"
#include "FixPyRunner.h"

// Config widget used by the Preferences/Options config panes
class FixPyDataProcessorConfig : public DataProcessorConfig
{
public:
    // there is no config
    FixPyDataProcessorConfig(QWidget *parent) : DataProcessorConfig(parent) {}
    QString explain() { return QString(); }
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


    // Creating a RideItem object from rideFile, to substitute the current one in context object,
    // so that activityMetrics() can access its data

    QDateTime dt = rideFile->startTime();
    RideItem *ride = new RideItem(rideFile, dt, rideFile->context);
    RideItem *prior = rideFile->context->ride;
    // It must be refreshed to have metadata, etc updated and accesible from within python code
    ride->refresh();
    rideFile->context->ride = ride;
    FixPyRunner pyRunner(rideFile->context, rideFile, useNewThread);
    bool result = pyRunner.run(pyScript->source, pyScript->iniKey, errText) == 0;
    rideFile->context->ride = prior;
    return result;
}

DataProcessorConfig *FixPyDataProcessor::processorConfig(QWidget *parent, const RideFile* ride)
{
    Q_UNUSED(ride);
    return new FixPyDataProcessorConfig(parent);
}
