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

#ifndef _GC_FilterEditor_h
#define _GC_FilterEditor_h


#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QStringListModel>
#include <QCompleter>
#include <QKeyEvent>

#include <utility>


class FilterEditorHelper
{
public:
    FilterEditorHelper();
    virtual ~FilterEditorHelper();

    std::pair<QString, int> trimLeftOfCursor(const QString &t, int cp) const;
    std::pair<QString, int> trimRightOfCursor(const QString &t, int cp) const;
    QString wordLeftOfCursor(const QString &t, int cp) const;
    QString wordRightOfCursor(const QString &t, int cp) const;
    bool isSelectorPosition(const QString &t, int cp) const;
    bool isQuoted(const QString &t, int cp) const;
    bool isWordChar(QChar ch) const;

    std::pair<QString, int> evaluateCompletion(const QString &originalText, const QString &completion, int cursorPosition, int selectionStart, int selectionLength) const;
};


class FilterEditor: public QLineEdit
{
    Q_OBJECT

public:
    FilterEditor(QWidget *parent = nullptr);
    FilterEditor(const QString &contents, QWidget *parent = nullptr);
    virtual ~FilterEditor();

    void setFilterCommands(const QStringList &commands);
    QCompleter *completer() const;

protected:
    void setCompleter(QCompleter *completer);
    void keyPressEvent(QKeyEvent *e) override;

private slots:
    void insertCompletion(const QString &completion);
    void updateCompleterModel(const QString &text);

private:
    QCompleter *_completer;
    QStringListModel *_completerModel;
    QStringList _origCmds;
    FilterEditorHelper feh;

    QString wordLeftOfCursor() const;
    QString wordRightOfCursor() const;
    bool isSelectorPosition() const;
    bool isQuoted() const;
};

#endif
