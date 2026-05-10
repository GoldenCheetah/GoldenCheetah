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


RSyntax::RSyntax(QTextDocument *parent, bool dark) : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    assignmentFormat.setFontWeight(QFont::Normal);
    assignmentFormat.setForeground(dark ? QColor("#FFFFFF"): QColor("#1A1A1A"));
    rule.pattern = QRegularExpression("<{1,2}[-]");
    rule.format = assignmentFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("[-]>{1,2}");
    rule.format = assignmentFormat;
    highlightingRules.append(rule);


    // function: anything followed by (
    functionFormat.setFontWeight(QFont::Normal);
    functionFormat.setForeground(dark ? QColor("#79b8ff") : QColor("#007a99"));
    //functionFormat.setFontItalic(true);
    rule.pattern = QRegularExpression("\\b[A-Za-z0-9_\\.]+(?=[ ]*\\()");
    rule.format = functionFormat;
    highlightingRules.append(rule);

    // argument: anything followed by =, but not at the start(???)
    // unfortunately look behind assertions are not supported
    argumentFormat.setFontWeight(QFont::Normal);
    argumentFormat.setForeground(dark ? QColor("#9d8fcc") : QColor("#3d1a80"));
    rule.pattern = QRegularExpression("[A-Za-z0-9_\\.]+(?=[ ]*=[^=])");
    rule.format = argumentFormat;
    highlightingRules.append(rule);

    // numbers
    numberFormat.setFontWeight(QFont::Normal);
    numberFormat.setForeground(dark ? QColor("#4A7A3A") : QColor("#6A9955"));
    QStringList numberPatterns;
    numberPatterns << "\\b[0-9]+[\\.]?[0-9]*\\b"  << "\\b[\\.][0-9]+\\b";
    foreach (QString pattern, numberPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = numberFormat;
        highlightingRules.append(rule);
    }

    // constants: TRUE FALSE NA NULL Inf NaN
    constantFormat.setFontWeight(QFont::DemiBold);
    constantFormat.setForeground(dark ? QColor("#9d8fcc") : QColor("#3d1a80"));
    QStringList constantPatterns;
    constantPatterns << "\\bTRUE\\b" << "\\bFALSE\\b" << "\\bNA\\b"  << "\\bNULL\\b" << "\\bInf\\b" << "\\bNaN\\b";
    foreach (QString pattern, constantPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = constantFormat;
        highlightingRules.append(rule);
    }

    // keywords: while for in repeat if else switch break next
    // function return message warning stop
    keywordFormat.setFontWeight(QFont::Bold);
    keywordFormat.setForeground(dark ? QColor("#FFFFFF"): QColor("#1A1A1A"));
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
    commonFunctionFormat.setFontWeight(QFont::Bold);
    commonFunctionFormat.setForeground(dark ? QColor("#79b8ff") : QColor("#007a99"));
    QStringList commonFunctionPatterns;
    commonFunctionPatterns << "\\blibrary\\b" << "\\bsource\\b" << "\\brequire\\b";
    foreach (QString pattern, commonFunctionPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = commonFunctionFormat;
        highlightingRules.append(rule);
    }

    // operators
    operatorFormat.setFontWeight(QFont::Normal);
    operatorFormat.setForeground(dark ? QColor("#FFFFFF"): QColor("#1A1A1A"));
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
    quotationFormat.setFontWeight(QFont::DemiBold);
    quotationFormat.setForeground(dark ? QColor("#c38418") : QColor("#865910"));
    rule.pattern = QRegularExpression("\"[^\"]*\"");
    rule.format = quotationFormat;
    highlightingRules.append(rule);
    rule.pattern = QRegularExpression("'[^']*\'");
    rule.format = quotationFormat;
    highlightingRules.append(rule);

    // comments (should override everything else)
    commentFormat.setFontWeight(QFont::Normal);
    commentFormat.setForeground(dark ? QColor("#6a737d") : QColor("#8c959e"));
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
