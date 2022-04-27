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
    Context* context = (rideFile) ? rideFile->context : nullptr;
    FixPyRunner pyRunner(context, rideFile, useNewThread);
    return pyRunner.run(pyScript->source, pyScript->iniKey, errText) == 0;
}

DataProcessorConfig *FixPyDataProcessor::processorConfig(QWidget *parent, const RideFile* ride)
{
    Q_UNUSED(ride);
    return new FixPyDataProcessorConfig(parent);
}
