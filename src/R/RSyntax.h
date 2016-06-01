/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#ifndef GC_RSyntax
#define GC_RSyntax

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

class QTextDocument;

class RSyntax : public QSyntaxHighlighter
{
    Q_OBJECT

 public:
    RSyntax(QTextDocument *parent = 0);

 protected:
    void highlightBlock(const QString &text);

 private:
    struct HighlightingRule {
        QRegExp pattern;
        QTextCharFormat format;
    };
    QVector<HighlightingRule> highlightingRules;

    // initial classifications and values borrowed from ESS
    QTextCharFormat assignmentFormat;
    QTextCharFormat functionFormat;
    QTextCharFormat numberFormat;
    QTextCharFormat argumentFormat;
    QTextCharFormat constantFormat;
    QTextCharFormat keywordFormat;
    QTextCharFormat commonFunctionFormat;
    QTextCharFormat operatorFormat;
    QTextCharFormat namespaceFormat;
    QTextCharFormat commentFormat;
    QTextCharFormat quotationFormat;
};

#endif
