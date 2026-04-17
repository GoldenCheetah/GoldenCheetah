/*
 * Copyright (c) 2024 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "SplashScreen.h"

#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QApplication>
#include <QScreen>

#include "Colors.h"
#include "GcUpgrade.h"

SplashScreen::SplashScreen
()
: QSplashScreen()
{
    const QString textRgb("#000000");
    const QPixmap logoPixmap(":/images/gc.png");

    int imageSize = 280;
    if (QApplication::primaryScreen()->geometry().width() > 1280 && dpiXFactor > 1) {
        imageSize = qRound(imageSize * dpiXFactor);
    }
    const int versionHeight = qRound(imageSize * 0.08);
    const int padding = qRound(imageSize * 0.04);
    const int pixmapWidth = imageSize;
    const int pixmapHeight = imageSize + padding + versionHeight;

    QPixmap pixmap(pixmapWidth, pixmapHeight);
    pixmap.fill(Qt::white);

    if (! logoPixmap.isNull()) {
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        const QPixmap scaled = logoPixmap.scaled(imageSize, imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int x = (pixmapWidth - scaled.width()) / 2;
        painter.drawPixmap(x, 0, scaled);
    }
    setPixmap(pixmap);

    const QString version = QString(tr("%1 - build %2")).arg(VERSION_STRING).arg(VERSION_LATEST);
    QLabel *versionLabel = new QLabel(this);
    versionLabel->setStyleSheet(QString("QLabel { color: %1; background-color: white; }").arg(textRgb));
    QFont f = versionLabel->font();
    f.setPixelSize(qRound(versionHeight * 0.8));
    f.setBold(true);
    versionLabel->setFont(f);
    versionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    versionLabel->setGeometry(0, imageSize + padding, pixmapWidth, versionHeight);
    versionLabel->setText(version);

    msgLabel = new QLabel(this);
    msgLabel->hide();

    show();
}

SplashScreen::~SplashScreen
()
{
    close();
}


void
SplashScreen::showMessage
(const QString &)
{
    QApplication::processEvents();
}


void
SplashScreen::clearMessage
()
{
    QApplication::processEvents();
}
