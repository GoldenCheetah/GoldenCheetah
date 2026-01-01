#include <QTest>
#include "testSignalSafety.cpp"
#include "testPatternDetection.cpp"
#include "testTreeSafety.cpp"

#include <QCoreApplication> // Added for QCoreApplication

int main(int argc, char *argv[])
{
    int status = 0;

    QCoreApplication app(argc, argv);

    {
        TestSignalSafety tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        TestPatternDetection tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    {
        TestTreeSafety tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
