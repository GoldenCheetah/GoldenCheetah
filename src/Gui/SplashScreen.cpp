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

#include "Colors.h"
#include "GcUpgrade.h"

#define SHOW_BOX 0 // 1 shows labels background (useful for fitting)


SplashScreen::SplashScreen
()
: QSplashScreen()
{
    const QString textRgb("#000000");
    int versionX = 48;
    int versionY = 296;
    int versionWidth = 243;
    int versionHeight = 21;
    int sepOverlap = 2;
    int msgX = versionX;
    int msgY = 320;
    int msgWidth = versionWidth;
    int msgHeight = 14;
    const QString pixmapPath(":images/splashscreen.png");
    const QString version = QString(tr("%1 - build %2")).arg(VERSION_STRING).arg(VERSION_LATEST);

    setAttribute(Qt::WA_TranslucentBackground);

    QPixmap pixmap(pixmapPath);
    int targetPixmapWidth = -1;
    if (QApplication::primaryScreen()->geometry().width() <= 1280) {
        targetPixmapWidth = 280;
    } else if (dpiXFactor > 1) {
        targetPixmapWidth = pixmap.rect().width() * dpiXFactor;
    }
    if (targetPixmapWidth > 0) {
        // Scale pixmap for low or high screen resolutions
        int origPixmapWidth = pixmap.rect().width();
        int origPixmapHeight = pixmap.rect().height();
        pixmap = pixmap.scaledToWidth(targetPixmapWidth, Qt::SmoothTransformation);
        double xFactor = pixmap.rect().width() / static_cast<double>(origPixmapWidth);
        double yFactor = pixmap.rect().height() / static_cast<double>(origPixmapHeight);
        versionX *= xFactor;
        versionY *= yFactor;
        versionWidth *= xFactor;
        versionHeight *= yFactor;
        sepOverlap *= xFactor;
        msgX *= xFactor;
        msgY *= yFactor;
        msgWidth *= xFactor;
        msgHeight *= yFactor;
    }
    if (version.size() > 0) {
        QLabel *versionLabel = new QLabel(this);
#if SHOW_BOX == 0
        versionLabel->setAttribute(Qt::WA_TranslucentBackground);
        versionLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(textRgb));
#else
        versionLabel->setStyleSheet("QLabel { color: white; background-color: darkgreen; }");
#endif
        QFont f = versionLabel->font();
        f.setPixelSize(versionHeight * 0.8);
        f.setBold(true);
        versionLabel->setFont(f);
        versionLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        versionLabel->setGeometry(versionX, versionY, versionWidth, versionHeight);
        versionLabel->setText(version);

        int sepLeft = versionX - sepOverlap;
        int sepRight = versionX + versionWidth + sepOverlap;
        int sepY = (versionY + versionHeight + msgY) / 2;
        QPainter painter(&pixmap);
        painter.setPen(QColor(textRgb));
        painter.drawLine(sepLeft, sepY, sepRight, sepY);
    }
    setPixmap(pixmap);

    msgLabel = new QLabel(this);
#if SHOW_BOX == 0
    msgLabel->setAttribute(Qt::WA_TranslucentBackground);
    msgLabel->setStyleSheet(QString("QLabel { color: %1; }").arg(textRgb));
#else
    msgLabel->setStyleSheet("QLabel { color: white; background-color: darkred; }");
#endif
    QFont f = msgLabel->font();
    f.setPixelSize(msgHeight * 0.8);
    msgLabel->setFont(f);
    msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    msgLabel->setGeometry(msgX, msgY, msgWidth, msgHeight);

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
    msgLabel->setText(msg);
    QApplication::processEvents();
}


void
SplashScreen::clearMessage
()
{
    msgLabel->clear();
    QApplication::processEvents();
}
