#include "Core/Utils.h"

#include <QTest>


class TestUtils: public QObject
{
    Q_OBJECT

private slots:
    void quoteEscapeTest() {
        QCOMPARE(Utils::quoteEscape("abc"), QString("abc"));
        QCOMPARE(Utils::quoteEscape("a\bc"), QString("a\bc"));
        QCOMPARE(Utils::quoteEscape("a\\bc"), QString("a\\bc"));
        QCOMPARE(Utils::quoteEscape("a\"bc"), QString("a\\\"bc"));
        QCOMPARE(Utils::quoteEscape("a\\\"bc"), QString("a\\\"bc"));
        QCOMPARE(Utils::quoteEscape("a\\\\\"bc"), QString("a\\\\\\\"bc"));
    }
};


QTEST_MAIN(TestUtils)
#include "testUtils.moc"
