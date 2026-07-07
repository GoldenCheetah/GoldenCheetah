#include <QTest>
#include <QObject>
#include <QSignalSpy>

class Sender : public QObject
{
    Q_OBJECT
signals:
    void theSignal();
};

class Receiver : public QObject
{
    Q_OBJECT
public:
    int callCount = 0;
};

class TestSignalSafety : public QObject
{
    Q_OBJECT

private slots:
    void testMissingReceiver() {
        // This test mimics the "missing receiver" bug.
        // We want to show that if we DON'T provide a context object, the lambda is connected to the connection handle,
        // and if the "conceptual" receiver dies, the lambda still runs.
        // NOTE: In a real app this leads to a crash if the lambda captures 'this'.

        Sender sender;
        int callCount = 0;

        {
            Receiver receiver;
            // UNSAFE PATTERN: No context object.
            // The connection is tied to 'sender' life, not 'receiver' life.
            connect(&sender, &Sender::theSignal, [&]() {
                // accessing 'receiver' here after it dies would use dangling reference/pointer
                // For this test we just increment a counter to show it runs.
                callCount++;
            });

            // Emit while receiver is alive -> should run
            emit sender.theSignal();
            QCOMPARE(callCount, 1);
        } // receiver dies here

        // Emit after receiver died -> should STILL run (unsafe!)
        emit sender.theSignal();
        QCOMPARE(callCount, 2);
    }

    void testFixedPattern() {
        // This test mimics the fix.
        // We provide the context object (receiver). When it dies, the connection is auto-disconnected.

        Sender sender;
        int callCount = 0;

        {
            Receiver receiver;
            // SAFE PATTERN: Context object provided as 3rd arg.
            connect(&sender, &Sender::theSignal, &receiver, [&]() {
                callCount++;
            });

            // Emit while receiver is alive -> should run
            emit sender.theSignal();
            QCOMPARE(callCount, 1);
        } // receiver dies here -> connection should be removed

        // Emit after receiver died -> should NOT run
        emit sender.theSignal();
        QCOMPARE(callCount, 1);
    }
};

#include "testSignalSafety.moc"
