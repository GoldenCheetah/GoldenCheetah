/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "FilterEditor.h"

#include <QRect>
#include <QAbstractItemView>
#include <QScrollBar>


FilterEditor::FilterEditor
(QWidget *parent)
: QLineEdit(parent), _completer(nullptr), _completerModel(nullptr), _origCmds()
{
}


FilterEditor::FilterEditor
(const QString &contents, QWidget *parent)
: QLineEdit(contents, parent), _completer(nullptr), _completerModel(nullptr), _origCmds()
{
}


FilterEditor::~FilterEditor
()
{
}


void
FilterEditor::setFilterCommands
(const QStringList &commands)
{
    if (_completerModel == nullptr) {
        _completerModel = new QStringListModel();
    }
    _completerModel->setStringList(commands);
    setCompleter(new QCompleter(_completerModel));
    _origCmds.clear();
    _origCmds << commands;
    _origCmds.sort(Qt::CaseInsensitive);

    _completer->setCaseSensitivity(Qt::CaseInsensitive);
    _completer->setFilterMode(Qt::MatchContains);
    _completer->setCompletionMode(QCompleter::PopupCompletion);
    _completer->setWrapAround(false);
    connect(this, SIGNAL(textChanged(QString)), this, SLOT(updateCompleterModel(QString)));
}


void
FilterEditor::setCompleter
(QCompleter *completer)
{
    if (_completer) {
        _completer->disconnect(this);
    }
    _completer = completer;
    if (_completer == nullptr) {
        return;
    }

    _completer->setWidget(this);
    connect(_completer, QOverload<const QString &>::of(&QCompleter::activated), this, &FilterEditor::insertCompletion);
}


QCompleter*
FilterEditor::completer
() const
{
    return _completer;
}


void
FilterEditor::keyPressEvent
(QKeyEvent *e)
{
    if (   _completer
        && _completer->popup()->isVisible()
        && (   e->key() == Qt::Key_Enter
            || e->key() == Qt::Key_Return
            || e->key() == Qt::Key_Escape
            || e->key() == Qt::Key_Tab
            || e->key() == Qt::Key_Backtab)) {
        // The keys above are forwarded from the completer to the widget
        e->ignore();
        return;
    }
    const bool isShortcut = (e->modifiers().testFlag(Qt::ControlModifier) && e->key() == Qt::Key_E); // CTRL+E
    if (! _completer || ! isShortcut) {
        QLineEdit::keyPressEvent(e);
    }

    const bool ignoredModifier =    e->modifiers().testFlag(Qt::ShiftModifier)
                                 || e->modifiers().testFlag(Qt::MetaModifier);
    if (! _completer || (e->text().isEmpty() && ignoredModifier)) {
        return;
    }

    const bool hasModifier = (e->modifiers() != Qt::NoModifier) && ! ignoredModifier;
    if (   ! isShortcut
        && (   hasModifier
            || e->text().isEmpty())) {
        _completer->popup()->hide();
        return;
    }

    if (   e->text().endsWith(" ")
        || e->text().endsWith(",")
        || e->text().endsWith(".")) {
        _completer->popup()->hide();
        return;
    }
    if (! isSelectorPosition()) {
        return;
    }

    QString completionPrefix = wordLeftOfCursor();
    if (completionPrefix != _completer->completionPrefix()) {
        _completer->setCompletionPrefix(completionPrefix);
        int index = 0;
        for (int i = 0; i < _completer->completionModel()->rowCount(); ++i) {
            if (_completer->completionModel()->index(i, 0).data().toString().startsWith(completionPrefix)) {
                index = i;
                break;
            }
        }
        _completer->popup()->setCurrentIndex(_completer->completionModel()->index(index, 0));
    }
    QRect cr = cursorRect();
    cr.setWidth(_completer->popup()->sizeHintForColumn(0) + _completer->popup()->verticalScrollBar()->sizeHint().width());
    _completer->complete(cr);
}


void
FilterEditor::insertCompletion
(const QString &completion)
{
    if (_completer == nullptr || _completer->widget() != this) {
        return;
    }

    std::pair<QString, int> c = feh.evaluateCompletion(text(), completion, cursorPosition(), selectionStart(), selectionLength());
    setText(c.first);
    setCursorPosition(c.second);
}


void
FilterEditor::updateCompleterModel
(const QString &text)
{
    int cp = cursorPosition();
    int cpCmdIdx = 0;
    while (cp > 0) {
        --cp;
        if (text[cp] == ',') {
            ++cpCmdIdx;
        }
    }
    QStringList cmds;
    QStringList tokens = text.split(",");
    for (int i = 0; i < tokens.size(); ++i) {
        const auto &token = tokens[i];
        if (i == cpCmdIdx) {
            continue;
        }
        QStringList subTokens = token.trimmed().split(" ");
        if (subTokens.size() > 0) {
            QString subToken = subTokens[0].trimmed().toLower();
            if (subToken.size() > 0) {
                cmds << subToken;
            }
        }
    }
    QStringList completerCmds;
    for (const auto &origCmd : qAsConst(_origCmds)) {
        if (! cmds.contains(origCmd, Qt::CaseInsensitive)) {
            completerCmds << origCmd;
        }
    }
    _completerModel->setStringList(completerCmds);
}


QString
FilterEditor::wordLeftOfCursor
() const
{
    return feh.wordLeftOfCursor(text(), cursorPosition());
}


QString
FilterEditor::wordRightOfCursor
() const
{
    return feh.wordRightOfCursor(text(), cursorPosition());
}


bool
FilterEditor::isSelectorPosition
() const
{
    return feh.isSelectorPosition(text(), cursorPosition());
}


/// FilterEditorHelper

FilterEditorHelper::FilterEditorHelper
()
{
}


FilterEditorHelper::~FilterEditorHelper
()
{
}


std::pair<QString, int>
FilterEditorHelper::trimLeftOfCursor
(const QString &t, int cp) const
{
    if (t.size() == 0 || cp == 0) {
        return std::make_pair<QString, int>(QString(t), int(cp));
    }
    QString nt(t);
    int cursorpos = cp;
    int charpos = cp - 1;
    int spaces = 0;
    while (charpos >= 0 && nt[charpos].isSpace()) {
        --charpos;
        ++spaces;
    }
    if (charpos < 0) {
        charpos = 0;
        cursorpos = 0;
    } else {
        cursorpos = charpos + 1;
    }
    if (! nt[charpos].isSpace()) {
        ++charpos;
    }
    if (spaces > 0) {
        nt.remove(charpos, spaces);
    }

    return std::make_pair<QString, int>(QString(nt), int(cursorpos));
}


std::pair<QString, int>
FilterEditorHelper::trimRightOfCursor
(const QString &t, int cp) const
{
    if (cp >= t.size()) {
        return std::make_pair<QString, int>(QString(t), int(cp));
    }
    QString nt(t);
    int cursorpos = cp;
    int charpos = cp;
    int spaces = 0;
    while (charpos < nt.size() && nt[charpos].isSpace()) {
        ++charpos;
        ++spaces;
    }
    if (charpos >= nt.size()) {
        charpos = nt.size() - 1;
        cursorpos = nt.size() - spaces;
    } else {
        cursorpos = charpos - spaces;
    }
    if (! nt[charpos].isSpace()) {
        --charpos;
    }
    if (spaces > 0) {
        nt.remove(charpos - spaces + 1, spaces);
    }

    return std::make_pair<QString, int>(QString(nt), int(cursorpos));
}


QString
FilterEditorHelper::wordLeftOfCursor
(const QString &t, int cp) const
{
    QString left = t.left(cp);
    QString prefix;
    for (int i = left.size() - 1; i >= 0; --i) {
        QChar ch = left[i];
        if (! isWordChar(ch)) {
            break;
        }
        prefix.prepend(ch);
    }
    return prefix;
}


QString
FilterEditorHelper::wordRightOfCursor
(const QString &t, int cp) const
{
    int cursorPos = cp;
    QString right = t.right(t.size() - cursorPos);
    QString suffix;
    for (int i = 0; i < right.size(); ++i) {
        QChar ch = right[i];
        if (! isWordChar(ch)) {
            break;
        }
        suffix.append(ch);
    }
    return suffix;
}


bool
FilterEditorHelper::isSelectorPosition
(const QString &t, int cp) const
{
    int pos = cp - 1;
    while (pos > 0 && isWordChar(t[pos])) {
        --pos;
    }
    while (pos > 0) {
        if (t[pos].isSpace()) {
            --pos;
        } else if (t[pos] == ',') {
            return true;
        } else {
            return false;
        }
    }
    return true;
}


bool
FilterEditorHelper::isWordChar
(QChar ch) const
{
    return ! (   ch.isSpace()
              || ch == ','
              || ch == '.');
}


std::pair<QString, int>
FilterEditorHelper::evaluateCompletion
(const QString &originalText, const QString &completion, int cursorPosition, int selectionStart, int selectionLength) const
{
    QString modCompletion(completion);

    QString newText = originalText;
    int cp = cursorPosition;

    // Remove selection
    if (selectionStart != -1) {
        cp = selectionStart;
        newText.remove(selectionStart, selectionLength);
    }

    // Remove word under cursor
    QString loc = wordLeftOfCursor(newText, cp);
    if (loc.length() > 0) {
        cp -= loc.length();
        newText.remove(cp, loc.length());
    }
    QString roc = wordRightOfCursor(newText, cp);
    if (roc.length() > 0) {
        newText.remove(cp, roc.length());
    }

    // Seek left, removing spaces
    std::pair<QString, int> lefttrim = trimLeftOfCursor(newText, cp);
    newText = lefttrim.first;
    cp = lefttrim.second;
    if (cp > 0) {
        newText.insert(cp, " ");
        ++cp;
    }

    // Seek right, removing spaces
    std::pair<QString, int> righttrim = trimRightOfCursor(newText, cp);
    newText = righttrim.first;
    cp = righttrim.second;

    // Insert completed text
    newText.insert(cp, modCompletion);
    cp += modCompletion.size();

    // Insert trailing space
    if (newText[cp] != ' ') {
        newText.insert(cp, " ");
    }
    ++cp;

#if 0
    qDebug().noquote() << "FilterEditorHelper::evaluateCompletion:" << QString("QCOMPARE(feh.evaluateCompletion(\"%1\", \"%2\", \"%3\", %4, %5, %6), rp(\"%7\", %8));").arg(originalText).arg(completion).arg(cursorPosition).arg(selectionStart).arg(selectionLength).arg(newText).arg(cp);
#endif

    return std::make_pair<QString, int>(QString(newText), int(cp));
}
