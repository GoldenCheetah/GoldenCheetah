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

#include <QSyntaxHighlighter>

#include "RSyntax.h"


RSyntax::RSyntax(QTextDocument *parent) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    assignmentFormat.setForeground(QColor(255,204,000));
    rule.pattern = QRegularExpression("<{1,2}[-]");
    rule.format = assignmentFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("[-]>{1,2}");
    rule.format = assignmentFormat;
    highlightingRules.append(rule);


    // function: anything followed by (
    functionFormat.setForeground(Qt::cyan);
    //functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_\\.]+(?=[ ]*\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // argument: anything followed by =, but not at the start(???)
    // unfortunately look behind assertions are not supported
    argumentFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("[A-Za-z0-9_\\.]+(?=[ ]*=[^=])");
    rule.format = argumentFormat;
    highlightingRules.append(rule);

    // numbers
    numberFormat.setForeground(Qt::red);
    QStringList numberPatterns;
    numberPatterns << "\\b[0-9]+[\\.]?[0-9]*\\b"  << "\\b[\\.][0-9]+\\b";
    foreach (QString pattern, numberPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = numberFormat;
        highlightingRules.append(rule);
    }

    // constants: TRUE FALSE NA NULL Inf NaN
    constantFormat.setForeground(Qt::red);
    QStringList constantPatterns;
    constantPatterns << "\\bTRUE\\b" << "\\bFALSE\\b" << "\\bNA\\b"  << "\\bNULL\\b" << "\\bInf\\b" << "\\bNaN\\b";
    foreach (QString pattern, constantPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = constantFormat;
        highlightingRules.append(rule);
    }

    // keywords: while for in repeat if else switch break next
    // function return message warning stop
    keywordFormat.setForeground(QColor(255,204,000));
    //keywordFormat.setFontItalic(true);
    QStringList keywordPatterns;
    keywordPatterns << "\\bwhile\\b" << "\\bfor\\b" << "\\bin([^%]?)\\b" 
            << "\\brepeat\\b" << "\\bif\\b" << "\\belse\\b" 
            << "\\bswitch\\b" << "\\bbreak\\b" << "\\bnext\\b" 
            << "\\bfunction\\b" << "\\breturn\\b" << "\\bmessage\\b" 
            << "\\bwarning\\b" << "\\bstop\\b";
    foreach (QString pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    // common functions (says who? (I hate attach)): library attach
    // detach source require
    commonFunctionFormat.setForeground(QColor(255,204,000));
    QStringList commonFunctionPatterns;
    commonFunctionPatterns << "\\blibrary\\b" << "\\bsource\\b" << "\\brequire\\b";
    foreach (QString pattern, commonFunctionPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = commonFunctionFormat;
        highlightingRules.append(rule);
    }

    // operators
    operatorFormat.setForeground(QColor(255,204,000));
    //operatorFormat.setForeground(Qt::darkCyan);
    //operatorFormat.setFontWeight(QFont::Bold);
    QStringList operatorPatterns;

    //operatorPatterns << "[\\&\\$\\@\\|\\:\\~\\{\\}\\(\\)!]" << "==" << "!=";
    operatorPatterns << "[\\&\\@\\|\\:\\~!<>=]" << "==" << "!=";

    foreach (QString pattern, operatorPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = operatorFormat;
        highlightingRules.append(rule);
    }

    // namespace: anything followed by ::
    //namespaceFormat.setForeground(Qt::magenta);
    //rule.pattern = QRegularExpression("\\b[A-Za-z0-9_\\.]+(?=::)");
    //rule.format = namespaceFormat;
    //highlightingRules.append(rule);

    // quotes: only activated after quotes are closed.  Does not
    // span lines.
    quotationFormat.setForeground(Qt::red);
    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("'[^']*\'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // comments (should override everything else)
    commentFormat.setForeground(QColor(100,149,237));
    rule.pattern = QRegularExpression("#[^\n]*");
    rule.format = commentFormat;
    highlightingRules.append(rule);
}


void RSyntax::highlightBlock(const QString &text)
{
    foreach (HighlightingRule rule, highlightingRules) {
        QRegularExpression expression(rule.pattern);
        QRegularExpressionMatch match;
        int index = text.indexOf(expression, 0, &match);
        // NB: this is index in block, not full document
        while (index >= 0) {
            int length = match.capturedLength();
            setFormat(index, length, rule.format);
            index = text.indexOf(expression, index + length, &match);
        }
    }
}
