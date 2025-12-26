/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_IconManager_h
#define _GC_IconManager_h

#include <memory>

#include <QString>
#include <QHash>
#include <QDir>

#include "MainWindow.h"
#include "RideItem.h"


class IconManager {
public:
    static IconManager &instance();

    IconManager(const IconManager&) = delete;
    IconManager &operator=(const IconManager&) = delete;

    // Full path
    QString getFilepath(const QString &sport, const QString &subSport = "") const;
    QString getFilepath(RideItem const * const rideItem) const;
    QString getDefault() const;

    // Filename only
    QStringList listIconFiles() const;
    QString toFilepath(const QString &filename);

    QString assignedIcon(const QString &field, const QString &value) const;
    void assignIcon(const QString &field, const QString &value, const QString &filename);

    bool addIconFile(const QFile &sourceFile);
    bool deleteIconFile(const QString &filename);

    bool exportBundle(const QString &filepath);
    bool importBundle(const QString &filepath);
    bool importBundle(const QUrl &url);
    bool importBundle(std::unique_ptr<QIODevice> device);

    bool saveConfig() const;

private:
    IconManager();

    const QString defaultIcon = ":images/breeze/games-highscores.svg";
    QHash<QString, QHash<QString, QString>> icons;  // Key 1: Fieldname (Sport, SubSport, ...)
                                                    // Key 2: Value (Bike, Run, ...)
                                                    // Value: Icon filename (no path)

    const QDir baseDir = QDir(gcroot + "/.icons");
    const QString configFile = "mapping.json";

    bool loadMapping();
    void writeGroup(QJsonObject &rootObj, const QString &field, const QHash<QString, QString> &data) const;
    QHash<QString, QString> readGroup(const QJsonObject &rootObj, const QString &field);
    QByteArray downloadUrl(const QUrl &url, int timeoutMs = 15000);
};

#endif
