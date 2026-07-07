#include "qxtstringspinbox.h"
#include <QStyle>
#include <QStyleFactory>

/****************************************************************************
** Copyright (c) 2006 - 2011, the LibQxt project.
** See the Qxt AUTHORS file for a list of authors and copyright holders.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in the
**       documentation and/or other materials provided with the distribution.
**     * Neither the name of the LibQxt project nor the
**       names of its contributors may be used to endorse or promote products
**       derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
** ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
** WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
** DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
** (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
** LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
** ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
** <http://libqxt.org>  <foundation@libqxt.org>
*****************************************************************************/


class QxtStringSpinBoxPrivate : public QxtPrivate<QxtStringSpinBox>
{
public:
    QXT_DECLARE_PUBLIC(QxtStringSpinBox)
    int startsWith(const QString& start, QString& string) const;
    QStringList strings;
};

int QxtStringSpinBoxPrivate::startsWith(const QString& start, QString& string) const
{
    const int size = strings.size();
    for (int i = 0; i < size; ++i)
    {
        if (strings.at(i).startsWith(start, Qt::CaseInsensitive))
        {
            // found
            string = strings.at(i);
            return i;
        }
    }
    // not found
    return -1;
}

/*!
    \class QxtStringSpinBox
    \inmodule QxtWidgets
    \brief The QxtStringSpinBox widget is a spin box with string items.

    QxtStringSpinBox is spin box that takes strings. QxtStringSpinBox allows
    the user to choose a value by clicking the up and down buttons or by
    pressing Up or Down on the keyboard to increase or decrease the value
    currently displayed. The user can also type the value in manually.

    \image qxtstringspinbox.png "QxtStringSpinBox in Cleanlooks style."
 */

/*!
    Constructs a new QxtStringSpinBox with \a pParent.
 */
QxtStringSpinBox::QxtStringSpinBox(QWidget* pParent) : QSpinBox(pParent)
{
#if (!defined Q_OS_MAC)
    setStyle(QStyleFactory::create("fusion"));
#endif
    setRange(0, 0);
}

/*!
    Destructs the spin box.
 */
QxtStringSpinBox::~QxtStringSpinBox()
{}

/*!
    \property QxtStringSpinBox::strings
    \brief the string items of the spin box.
 */
const QStringList& QxtStringSpinBox::strings() const
{
    return qxt_d().strings;
}

void QxtStringSpinBox::setStrings(const QStringList& strings)
{
    qxt_d().strings = strings;
    setRange(0, strings.size() - 1);
    if (!strings.isEmpty())
        setValue(0);
}

/*!
    \reimp
 */
void QxtStringSpinBox::fixup(QString& input) const
{
    // just attempt to change the input string to be valid according to the string list
    // no need to result in a valid string, callers of this function are responsible to
    // re-test the validity afterwards

    // try finding a string from the list which starts with input
    input = input.simplified();
    if (!input.isEmpty())
    {
        qxt_d().startsWith(input, input);
    }
}

/*!
    \reimp
 */
QValidator::State QxtStringSpinBox::validate(QString& input, int& pos) const
{
    // Invalid:  input is invalid according to the string list
    // Intermediate: it is likely that a little more editing will make the input acceptable
    //   (e.g. the user types "A" and stringlist contains "ABC")
    // Acceptable:  the input is valid.
    Q_UNUSED(pos);
    QString temp;
    QValidator::State state = QValidator::Invalid;
    if (qxt_d().strings.contains(input))
    {
        // exact match
        state = QValidator::Acceptable;
    }
    else if (input.isEmpty() || qxt_d().startsWith(input, temp) != -1)
    {
        // still empty or some string in the list starts with input
        state = QValidator::Intermediate;
    }
    // else invalid
    return state;
}

/*!
    \reimp
 */
QString QxtStringSpinBox::textFromValue(int value) const
{
    Q_ASSERT(qxt_d().strings.isEmpty() || (value >= 0 && value < qxt_d().strings.size()));
    return qxt_d().strings.isEmpty() ? QLatin1String("") : qxt_d().strings.at(value);
}

/*!
    \reimp
 */
int QxtStringSpinBox::valueFromText(const QString& text) const
{
    return qxt_d().strings.indexOf(text);
}
