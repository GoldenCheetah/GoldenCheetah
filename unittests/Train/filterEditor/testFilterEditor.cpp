#include "Train/FilterEditor.h"

#include <QTest>

#define rp(s,c) (std::make_pair<QString, int>(s, c))


class TestFilterEditorHelper: public QObject
{
    Q_OBJECT

private slots:
    void evaluateCompletionEmpty();
    void evaluateCompletionLeft();
    void evaluateCompletionRight();
    void evaluateCompletionMiddle();
    void evaluateCompletionWrongPrefixEmpty();
    void evaluateCompletionWrongPrefixLeft();
    void evaluateCompletionWrongPrefixRight();
    void evaluateCompletionWrongPrefixMiddle();
    void evaluateCompletionSelectionAll();
    void evaluateCompletionSelectionLeft();
    void evaluateCompletionSelectionRight();
    void evaluateCompletionSelectionMiddle();

    void trimLeftOfCursor();
    void trimRightOfCursor();
};


void TestFilterEditorHelper::evaluateCompletionEmpty()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("p" , "power" , 1 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("p 100" , "power" , 1 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("p asd" , "power" , 1 , -1 , 0), rp("power asd", 6));
    QCOMPARE(feh.evaluateCompletion("powasd" , "power" , 1 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("powasd 100" , "power" , 1 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("powasd asd" , "power" , 1 , -1 , 0), rp("power asd", 6));
    QCOMPARE(feh.evaluateCompletion("  p" , "power" , 3 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("  p 100" , "power" , 3 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("  p asd" , "power" , 3 , -1 , 0), rp("power asd", 6));
}


void TestFilterEditorHelper::evaluateCompletionLeft()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("p," , "power" , 1 , -1 , 0), rp("power ,", 6));
    QCOMPARE(feh.evaluateCompletion("p 100," , "power" , 1 , -1 , 0), rp("power 100,", 6));
    QCOMPARE(feh.evaluateCompletion("p asd," , "power" , 1 , -1 , 0), rp("power asd,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd," , "power" , 1 , -1 , 0), rp("power ,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd 100," , "power" , 1 , -1 , 0), rp("power 100,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd asd," , "power" , 1 , -1 , 0), rp("power asd,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd, avgpower" , "power" , 1 , -1 , 0), rp("power , avgpower", 6));
    QCOMPARE(feh.evaluateCompletion("poasd 100, avgpower" , "power" , 1 , -1 , 0), rp("power 100, avgpower", 6));
    QCOMPARE(feh.evaluateCompletion("poasd asd, avgpower" , "power" , 1 , -1 , 0), rp("power asd, avgpower", 6));
}


void TestFilterEditorHelper::evaluateCompletionRight()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion(", p" , "power" , 3 , -1 , 0), rp(", power ", 8));
    QCOMPARE(feh.evaluateCompletion(", p 100" , "power" , 3 , -1 , 0), rp(", power 100", 8));
    QCOMPARE(feh.evaluateCompletion(", p asd" , "power" , 3 , -1 , 0), rp(", power asd", 8));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd" , "power" , 11 , -1 , 0), rp("avgpower, power ", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100" , "power" , 11 , -1 , 0), rp("avgpower, power 100", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd" , "power" , 11 , -1 , 0), rp("avgpower, power asd", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd" , "power" , 11 , -1 , 0), rp("avgpower, power ", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100" , "power" , 11 , -1 , 0), rp("avgpower, power 100", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd" , "power" , 11 , -1 , 0), rp("avgpower, power asd", 16));
}


void TestFilterEditorHelper::evaluateCompletionMiddle()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion(", p, " , "power" , 3 , -1 , 0), rp(", power , ", 8));
    QCOMPARE(feh.evaluateCompletion(",  p    100, " , "power" , 3 , -1 , 0), rp(", power 100, ", 8));
    QCOMPARE(feh.evaluateCompletion(", p asd," , "power" , 3 , -1 , 0), rp(", power asd,", 8));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power , minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power 100, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power asd, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power , minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power 100, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power asd, minpower", 16));
}


void TestFilterEditorHelper::evaluateCompletionWrongPrefixEmpty()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("p" , "power" , 1 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("p 100" , "power" , 1 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("p asd" , "power" , 1 , -1 , 0), rp("power asd", 6));
    QCOMPARE(feh.evaluateCompletion("p" , "power" , 1 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("p 100" , "power" , 1 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("p asd" , "power" , 1 , -1 , 0), rp("power asd", 6));
    QCOMPARE(feh.evaluateCompletion("powasd" , "power" , 1 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("powasd 100" , "power" , 1 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("powasd asd" , "power" , 1 , -1 , 0), rp("power asd", 6));
    QCOMPARE(feh.evaluateCompletion("  p" , "power" , 3 , -1 , 0), rp("power ", 6));
    QCOMPARE(feh.evaluateCompletion("  p 100" , "power" , 3 , -1 , 0), rp("power 100", 6));
    QCOMPARE(feh.evaluateCompletion("  p asd" , "power" , 3 , -1 , 0), rp("power asd", 6));
}


void TestFilterEditorHelper::evaluateCompletionWrongPrefixLeft()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("p," , "power" , 1 , -1 , 0), rp("power ,", 6));
    QCOMPARE(feh.evaluateCompletion("p 100," , "power" , 1 , -1 , 0), rp("power 100,", 6));
    QCOMPARE(feh.evaluateCompletion("p asd," , "power" , 1 , -1 , 0), rp("power asd,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd," , "power" , 1 , -1 , 0), rp("power ,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd 100," , "power" , 1 , -1 , 0), rp("power 100,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd asd," , "power" , 1 , -1 , 0), rp("power asd,", 6));
    QCOMPARE(feh.evaluateCompletion("poasd, avgpower" , "power" , 1 , -1 , 0), rp("power , avgpower", 6));
    QCOMPARE(feh.evaluateCompletion("poasd 100, avgpower" , "power" , 1 , -1 , 0), rp("power 100, avgpower", 6));
    QCOMPARE(feh.evaluateCompletion("poasd asd, avgpower" , "power" , 1 , -1 , 0), rp("power asd, avgpower", 6));
}


void TestFilterEditorHelper::evaluateCompletionWrongPrefixRight()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion(", p" , "power" , 3 , -1 , 0), rp(", power ", 8));
    QCOMPARE(feh.evaluateCompletion(", p 100" , "power" , 3 , -1 , 0), rp(", power 100", 8));
    QCOMPARE(feh.evaluateCompletion(", p asd" , "power" , 3 , -1 , 0), rp(", power asd", 8));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd" , "power" , 11 , -1 , 0), rp("avgpower, power ", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100" , "power" , 11 , -1 , 0), rp("avgpower, power 100", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd" , "power" , 11 , -1 , 0), rp("avgpower, power asd", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd" , "power" , 11 , -1 , 0), rp("avgpower, power ", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100" , "power" , 11 , -1 , 0), rp("avgpower, power 100", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd" , "power" , 11 , -1 , 0), rp("avgpower, power asd", 16));
}


void TestFilterEditorHelper::evaluateCompletionWrongPrefixMiddle()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion(", p, " , "power" , 3 , -1 , 0), rp(", power , ", 8));
    QCOMPARE(feh.evaluateCompletion(",  p    100, " , "power" , 3 , -1 , 0), rp(", power 100, ", 8));
    QCOMPARE(feh.evaluateCompletion(", p asd," , "power" , 3 , -1 , 0), rp(", power asd,", 8));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power , minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power 100, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power asd, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power , minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd 100, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power 100, minpower", 16));
    QCOMPARE(feh.evaluateCompletion("avgpower, poasd asd, minpower" , "power" , 11 , -1 , 0), rp("avgpower, power asd, minpower", 16));
}


void TestFilterEditorHelper::evaluateCompletionSelectionAll()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("power 100", "avgpower", 0, 0, 9), rp("avgpower ", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100", "avgpower", 0, 0, 23), rp("avgpower ", 9));
}


void TestFilterEditorHelper::evaluateCompletionSelectionLeft()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "avgpower", 0, 0, 2), rp("avgpower 100, avgpower 100", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100", "avgpower", 0, 0, 8), rp("avgpower , minpower 100", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100", "avgpower", 0, 0, 10), rp("avgpower minpower 100", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "avgpower", 1, 0, 2), rp("avgpower 100, avgpower 100", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100", "avgpower", 1, 0, 8), rp("avgpower , minpower 100", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100", "avgpower", 1, 0, 10), rp("avgpower minpower 100", 9));
}


void TestFilterEditorHelper::evaluateCompletionSelectionRight()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "duration", 15, 15, 8), rp("power 100, duration ", 20));
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "duration", 3, 3, 20), rp("duration ", 9));
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "created", 11, 11, 12), rp("power 100, created ", 19));
    QCOMPARE(feh.evaluateCompletion("power 100, created 100", "avgpower", 7, 7, 15), rp("power avgpower ", 15));
    QCOMPARE(feh.evaluateCompletion("power 100, avgpower 100", "created", 3, 3, 19), rp("created ", 8));
}


void TestFilterEditorHelper::evaluateCompletionSelectionMiddle()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.evaluateCompletion("power 100, minpower 100, maxpower 100", "created", 13, 13, 4), rp("power 100, created 100, maxpower 100", 19));
    QCOMPARE(feh.evaluateCompletion("power 100, created 100, maxpower 100", "distance", 13, 13, 8), rp("power 100, distance , maxpower 100", 20));
    QCOMPARE(feh.evaluateCompletion("power 100, distance 100, maxpower 100", "dominantzone", 12, 12, 10), rp("power 100, dominantzone , maxpower 100", 24));
    QCOMPARE(feh.evaluateCompletion("power 100, dominantzone z4, maxpower 100", "created", 12, 12, 17), rp("power 100, created 100", 19));
    QCOMPARE(feh.evaluateCompletion("power 100, created 100, distance 200", "duration", 2, 2, 18), rp("duration , distance 200", 9));
}


void TestFilterEditorHelper::trimLeftOfCursor()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.trimLeftOfCursor("", 0), rp("", 0));
    QCOMPARE(feh.trimLeftOfCursor(" ", 0), rp(" ", 0));
    QCOMPARE(feh.trimLeftOfCursor(" ", 1), rp("", 0));
    QCOMPARE(feh.trimLeftOfCursor("  ", 0), rp("  ", 0));
    QCOMPARE(feh.trimLeftOfCursor("  ", 1), rp(" ", 0));
    QCOMPARE(feh.trimLeftOfCursor("  ", 2), rp("", 0));
    QCOMPARE(feh.trimLeftOfCursor("a", 0), rp("a", 0));
    QCOMPARE(feh.trimLeftOfCursor("a ", 1), rp("a ", 1));
    QCOMPARE(feh.trimLeftOfCursor("a ", 2), rp("a", 1));
    QCOMPARE(feh.trimLeftOfCursor("a  ", 0), rp("a  ", 0));
    QCOMPARE(feh.trimLeftOfCursor("a  ", 1), rp("a  ", 1));
    QCOMPARE(feh.trimLeftOfCursor("a  ", 2), rp("a ", 1));
    QCOMPARE(feh.trimLeftOfCursor("a  ", 3), rp("a", 1));
}


void TestFilterEditorHelper::trimRightOfCursor()
{
    FilterEditorHelper feh;
    QCOMPARE(feh.trimRightOfCursor("", 0), rp("", 0));
    QCOMPARE(feh.trimRightOfCursor(" ", 0), rp("", 0));
    QCOMPARE(feh.trimRightOfCursor(" ", 1), rp(" ", 1));
    QCOMPARE(feh.trimRightOfCursor("  ", 0), rp("", 0));
    QCOMPARE(feh.trimRightOfCursor("  ", 1), rp(" ", 1));
    QCOMPARE(feh.trimRightOfCursor("  ", 2), rp("  ", 2));
    QCOMPARE(feh.trimRightOfCursor("a", 0), rp("a", 0));
    QCOMPARE(feh.trimRightOfCursor("a ", 1), rp("a", 1));
    QCOMPARE(feh.trimRightOfCursor("a ", 2), rp("a ", 2));
    QCOMPARE(feh.trimRightOfCursor("a  ", 0), rp("a  ", 0));
    QCOMPARE(feh.trimRightOfCursor("a  ", 1), rp("a", 1));
    QCOMPARE(feh.trimRightOfCursor("a  ", 2), rp("a ", 2));
    QCOMPARE(feh.trimRightOfCursor("a  ", 3), rp("a  ", 3));
    QCOMPARE(feh.trimRightOfCursor("ab  ", 1), rp("ab  ", 1));
    QCOMPARE(feh.trimRightOfCursor("a b  ", 1), rp("ab  ", 1));
    QCOMPARE(feh.trimRightOfCursor("a  b  ", 1), rp("ab  ", 1));
    QCOMPARE(feh.trimRightOfCursor("a   b  ", 1), rp("ab  ", 1));
}


QTEST_MAIN(TestFilterEditorHelper)
#include "testFilterEditor.moc"
