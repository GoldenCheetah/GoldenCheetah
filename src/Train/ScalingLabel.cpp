/*
 * Copyright (c) 2022 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "ScalingLabel.h"

#include <QDebug>


ScalingLabel::ScalingLabel
(QWidget *parent, Qt::WindowFlags f)
: ScalingLabel(4, 64, parent, f)
{
}



ScalingLabel::ScalingLabel
(int minFontPointSize, int maxFontPointSize, QWidget *parent, Qt::WindowFlags f)
: QLabel(parent, f), minFontPointSize(minFontPointSize), maxFontPointSize(maxFontPointSize)
{
    QFont fnt = font();
    fnt.setPointSize(minFontPointSize);
    QLabel::setFont(fnt);
}


ScalingLabel::~ScalingLabel
()
{
}


void
ScalingLabel::resizeEvent
(QResizeEvent *evt)
{
    Q_UNUSED(evt);
    scaleFont(text(), ScalingLabelReason::ResizeEvent);
}


void
ScalingLabel::setText
(const QString &text)
{
    ++counter;
    if (text.length() > QLabel::text().length()) {
        scaleFont(text, ScalingLabelReason::TextLengthChanged);
    } else if (counter > 10) {
        scaleFont(text, ScalingLabelReason::CounterExceeded);
    }
    QLabel::setText(text);
}


void
ScalingLabel::setFont
(const QFont &font)
{
    if (! scaleFont(text(), font, ScalingLabelReason::FontChanged)) {
        QLabel::setFont(font);
    }
}


int
ScalingLabel::getMinFontPointSize
() const
{
    return minFontPointSize;
}


void
ScalingLabel::setMinFontPointSize
(int size)
{
    minFontPointSize = size;
}


int
ScalingLabel::getMaxFontPointSize
() const
{
    return maxFontPointSize;
}


void
ScalingLabel::setMaxFontPointSize
(int size)
{
    maxFontPointSize = size;
}


bool
ScalingLabel::isLinear
() const
{
    return linear;
}


void
ScalingLabel::setLinear
(bool linear)
{
    this->linear = linear;
}


bool
ScalingLabel::scaleFont
(const QString &text, ScalingLabelReason reason)
{
    return scaleFont(text, font(), reason);
}


bool
ScalingLabel::scaleFont
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    counter = 0;
    if (linear) {
        return scaleFontLinear(text, font, reason);
    } else {
        return scaleFontExact(text, font, reason);
    }
}


bool
ScalingLabel::scaleFontExact
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    int size = maxFontPointSize + 1;
    if (reason == ScalingLabelReason::CounterExceeded) {
        size = font.pointSize() + 1;
    }
    QFont f(font);
    QRect br;
    do {
        f.setPointSize(--size);
        QFontMetrics fm = QFontMetrics(f, this);
        br = fm.boundingRect(text);
    } while (size >= minFontPointSize && (br.width() > width() || br.height() > height()));
    QLabel::setFont(f);
    return true;
}


bool
ScalingLabel::scaleFontLinear
(const QString &text, const QFont &font, ScalingLabelReason reason)
{
    if (text.length() == 0 || width() <= 0 || height() <= 0) {
        return false;
    }
    int maxSize = maxFontPointSize;
    if (reason == ScalingLabelReason::CounterExceeded) {
        maxSize = font.pointSize();
    }
    QFont f(font);
    f.setPointSize(minFontPointSize);
    QFontMetrics fmS = QFontMetrics(f, this);
    f.setPointSize(maxSize);
    QFontMetrics fmL = QFontMetrics(f, this);
    QRect brS = fmS.boundingRect(text);
    QRect brL = fmL.boundingRect(text);
    int sizeWidth = (maxSize - minFontPointSize) / float(brL.width() - brS.width()) * width();
    int sizeHeight = (maxSize - minFontPointSize) / float(brL.height() - brS.height()) * height();
    int calcSize = std::min<int>(sizeWidth, sizeHeight);
    f.setPointSize(std::max<int>(std::min<int>(calcSize, maxSize), minFontPointSize));
    QLabel::setFont(f);
    return true;
}
