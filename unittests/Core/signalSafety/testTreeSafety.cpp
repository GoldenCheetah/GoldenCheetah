#include <QTest>
#include <QProcess>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <iostream>

static QString repoRootForTreeSafety()
{
    const QString testSource = QFINDTESTDATA("testTreeSafety.cpp");
    if (!testSource.isEmpty()) {
        QDir dir(QFileInfo(testSource).absoluteDir());
        if (dir.cd("../../..")) return dir.absolutePath();
    }

    QDir dir(QCoreApplication::applicationDirPath());
    if (dir.cd("../../..")) return dir.absolutePath();

    return {};
}

class TestTreeSafety : public QObject
{
    Q_OBJECT

private slots:
    void testUnsafeChildAccess() {
        const QString repoRoot = repoRootForTreeSafety();
        QVERIFY2(!repoRoot.isEmpty(), "Could not locate repository root.");

        const QString scriptPath = QDir(repoRoot).filePath("util/check_unsafe_tree_child.py");
        const QString srcPath = QDir(repoRoot).filePath("src");
        QVERIFY2(QFile::exists(scriptPath), qPrintable(QString("Missing script: %1").arg(scriptPath)));

        QProcess process;
        QStringList args;
        args << scriptPath << srcPath;

        process.start("python3", args);
        bool started = process.waitForStarted();
        QVERIFY2(started, "Failed to start scripts");

        bool finished = process.waitForFinished(30000);
        QVERIFY2(finished, "Script timed out");

        int exitCode = process.exitCode();
        const QByteArray stdOut = process.readAllStandardOutput();
        if (exitCode != 0) {
            std::cout << stdOut.toStdString() << std::endl;
            std::cerr << process.readAllStandardError().toStdString() << std::endl;
        }

        QCOMPARE(exitCode, 0);
    }
};

#include "testTreeSafety.moc"
