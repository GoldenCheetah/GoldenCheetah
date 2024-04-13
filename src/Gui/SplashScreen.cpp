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

#include <QDebug>
#include <QPixmap>
#include <QPainter>
#include <QFont>
#include <QFontMetrics>
#include <QApplication>
#include <QScreen>


SplashScreen::SplashScreen
(const QString &pixmapPath, const QString &version, int versionX, int versionY)
: QSplashScreen()
{
    QPixmap pixmap(pixmapPath);
    QScreen *screen = QApplication::primaryScreen();
    if (screen->geometry().width() <= 1280) {
        // Scale pixmap for lower screen resolutions
        int targetPixmapWidth = 420;
        if (screen->geometry().width() < 1024) {
            targetPixmapWidth = 320;
        }
        int origPixmapWidth = pixmap.rect().width();
        int origPixmapHeight = pixmap.rect().height();
        pixmap = pixmap.scaledToWidth(targetPixmapWidth, Qt::SmoothTransformation);
        int newPixmapWidth = pixmap.rect().width();
        int newPixmapHeight = pixmap.rect().height();
        versionX *= newPixmapWidth / static_cast<double>(origPixmapWidth);
        versionY *= newPixmapHeight / static_cast<double>(origPixmapHeight);
    }
    QPainter painter(&pixmap);
    if (version.size() > 0) {
        QFont f = font();
        f.setPointSize(16);
        painter.setFont(f);
        QFontMetrics fm(f);
        QRect versionRect = fm.boundingRect(version);
        painter.drawText(versionX - versionRect.width(), versionY + versionRect.height(), version);
    }
    setPixmap(pixmap);

    show();
}

SplashScreen::~SplashScreen
()
{
    close();
}


void
SplashScreen::showMessage
(const QString &msg)
{
    QSplashScreen::showMessage(msg);
    QApplication::processEvents();
}


void
SplashScreen::clearMessage
()
{
    QSplashScreen::clearMessage();
    QApplication::processEvents();
}
