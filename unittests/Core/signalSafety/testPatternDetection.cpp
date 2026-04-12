#include <QTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <iostream>

static QString repoRootForPatternDetection()
{
    const QString testSource = QFINDTESTDATA("testPatternDetection.cpp");
    if (!testSource.isEmpty()) {
        QDir dir(QFileInfo(testSource).absoluteDir());
        if (dir.cd("../../..")) return dir.absolutePath();
    }

    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.cd("../../..")) return dir.absolutePath();

    return {};
}

class TestPatternDetection : public QObject
{
    Q_OBJECT

private slots:
    void testUnsafeConnects() {
        const QString repoRoot = repoRootForPatternDetection();
        QVERIFY2(!repoRoot.isEmpty(), "Could not locate repository root.");

        const QString scriptPath = QDir(repoRoot).filePath("util/check_unsafe_connects.py");
        const QString srcPath = QDir(repoRoot).filePath("src");
        QVERIFY2(QFile::exists(scriptPath), qPrintable(QString("Missing script: %1").arg(scriptPath)));

        QProcess process;
        QStringList args;
        args << scriptPath << srcPath;

        process.start("python3", args);
        bool started = process.waitForStarted();
        QVERIFY2(started, "Failed to start python3 process");

        bool finished = process.waitForFinished(30000); // 30 sec timeout
        QVERIFY2(finished, "Pattern detection script timed out");

        int exitCode = process.exitCode();

        if (exitCode != 0) {
            std::cout << process.readAllStandardOutput().toStdString() << std::endl;
            std::cerr << process.readAllStandardError().toStdString() << std::endl;
        }

        QCOMPARE(exitCode, 0);
    }
};

#include "testPatternDetection.moc"
