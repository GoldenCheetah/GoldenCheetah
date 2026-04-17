#include "FileIO/RideAutoImportConfig.h"

#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

namespace {

QList<RideAutoImportRule> buildRules()
{
    RideAutoImportRule rule;
    rule.setDirectory("/tmp/import & races");
    rule.setImportRule(RideAutoImportRule::importLast90days);
    return { rule };
}

} // namespace

class TestRideAutoImportConfig : public QObject
{
    Q_OBJECT

private slots:

    void readsRulesFromAutoImportXml()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString autoImportPath = tempDir.filePath("autoimport.xml");
        QVERIFY(RideAutoImportConfigParser::serialize(autoImportPath, buildRules()));

        RideAutoImportConfig config(QDir(tempDir.path()));
        QList<RideAutoImportRule> rules = config.getConfig();

        QCOMPARE(rules.size(), 1);
        QCOMPARE(rules.first().getDirectory(), QString("/tmp/import & races"));
        QCOMPARE(rules.first().getImportRule(), int(RideAutoImportRule::importLast90days));
    }

    void writeConfig_usesAutoImportXmlFilename()
    {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString autoImportPath = tempDir.filePath("autoimport.xml");
        const QString legacyPath = tempDir.filePath("autoimportrules.xml");
        QVERIFY(RideAutoImportConfigParser::serialize(autoImportPath, buildRules()));

        RideAutoImportConfig config(QDir(tempDir.path()));
        config.writeConfig();

        QVERIFY(QFileInfo::exists(autoImportPath));
        QVERIFY(!QFileInfo::exists(legacyPath));

        RideAutoImportConfig reloaded(QDir(tempDir.path()));
        QList<RideAutoImportRule> rules = reloaded.getConfig();

        QCOMPARE(rules.size(), 1);
        QCOMPARE(rules.first().getDirectory(), QString("/tmp/import & races"));
    }
};

QTEST_MAIN(TestRideAutoImportConfig)
#include "testRideAutoImportConfig.moc"
