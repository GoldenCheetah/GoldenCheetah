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
    Q_UNUSED(op);

    QString errText;
    FixPyRunner pyRunner(nullptr, rideFile);
    return pyRunner.run(pyScript->source, pyScript->iniKey, errText) == 0;
}

DataProcessorConfig *FixPyDataProcessor::processorConfig(QWidget *parent)
{
    return new FixPyDataProcessorConfig(parent);
}
