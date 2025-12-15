#include <QTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <iostream>

class TestPatternDetection : public QObject
{
    Q_OBJECT

private slots:
    void testUnsafeConnects() {
        // Find the script. We assume a relative path from the build dir or source dir.
        // The build dir is inside unittests/, so the script is in ../../util/check_unsafe_connects.py
        // We need to pass the source root (../../src) to it.

        QDir sourceDir(QCoreApplication::applicationDirPath());
        // Walk up from .obj/ or similar if needed, but typically we are in unittests/Core/signalSafety
        // Let's rely on relative paths from the project root if we launch from there,
        // OR construct it relative to the source tree.

        // This is a bit brittle depending on where the test is run from.
        // However, we can try to find the util directory.

        QString scriptPath = "../../../util/check_unsafe_connects.py";
        QString srcPath = "../../../src";

        // Check if script exists
        if (!QFile::exists(scriptPath)) {
             // Try another common location (if running from build dir deep structure)
             // We configured unittests to build in unittests/
             scriptPath = "../../../../util/check_unsafe_connects.py";
             srcPath = "../../../../src";
        }

        if (!QFile::exists(scriptPath)) {
            QSKIP("Could not find check_unsafe_connects.py script. Skipping pattern detection test.");
        }

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
