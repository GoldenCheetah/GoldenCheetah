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

#include "IconManager.h"

#include <QSvgRenderer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "../qzip/zipwriter.h"
#include "../qzip/zipreader.h"


IconManager&
IconManager::instance
()
{
    static IconManager instance;
    return instance;
}


IconManager::IconManager
()
{
    baseDir.mkpath(".");
    loadMapping();
}


QString
IconManager::getFilepath
(const QString &sport, const QString &subSport) const
{
    QString ret;
    if (   ! sport.isEmpty()
        && icons.contains("Sport")
        && ! icons["Sport"].value(sport, "").isEmpty()) {
        ret = baseDir.absoluteFilePath(icons["Sport"].value(sport, ""));
    }
    if (   ! subSport.isEmpty()
        && icons.contains("SubSport")
        && ! icons["SubSport"].value(subSport, "").isEmpty()) {
        ret = baseDir.absoluteFilePath(icons["SubSport"].value(subSport, ""));
    }
    QFileInfo fileInfo(ret);
    if (! fileInfo.isFile() || ! fileInfo.isReadable()) {
        ret = defaultIcon;
    }
    return ret;
}


QString
IconManager::getFilepath
(RideItem const * const rideItem) const
{
    if (rideItem != nullptr) {
        return getFilepath(rideItem->sport, rideItem->getText("SubSport", ""));
    }
    return defaultIcon;
}


QString
IconManager::getDefault
() const
{
    return defaultIcon;
}


QStringList
IconManager::listIconFiles
() const
{
    QStringList nameFilter { "*.svg" };
    return baseDir.entryList(nameFilter, QDir::Files | QDir::Readable);
}


QString
IconManager::toFilepath
(const QString &filename)
{
    return baseDir.absoluteFilePath(filename);
}


QString
IconManager::assignedIcon
(const QString &field, const QString &value) const
{
    if (icons.contains(field)) {
        return icons[field].value(value, "");
    }
    return "";
}


void
IconManager::assignIcon
(const QString &field, const QString &value, const QString &filename)
{
    if (! filename.isEmpty()) {
        icons[field][value] = filename;
    } else if (icons.contains(field)) {
        icons[field].remove(value);
    }
    saveConfig();
}


bool
IconManager::addIconFile
(const QFile &sourceFile)
{
    QFileInfo fileInfo(sourceFile);
    if (fileInfo.suffix() != "svg") {
        return false;
    }
    QSvgRenderer renderer(sourceFile.fileName());
    if (! renderer.isValid()) {
        return false;
    }
    return QFile::copy(fileInfo.absoluteFilePath(), baseDir.absoluteFilePath(fileInfo.fileName()));
}


bool
IconManager::deleteIconFile
(const QString &filename)
{
    QString filepath = toFilepath(filename);
    QFileInfo fileInfo(filepath);
    if (fileInfo.suffix() != "svg") {
        return false;
    }
    if (QFile::remove(filepath)) {
        bool save = false;
        for (QString &field : icons.keys()) {
            for (QString &value : icons[field].keys()) {
                if (icons[field].value(value, "") == filename) {
                    save = true;
                    icons[field].remove(value);
                }
            }
        }
        if (save) {
            saveConfig();
        }
        return true;
    } else {
        return false;
    }
}


bool
IconManager::exportBundle
(const QString &filename)
{
    QFile zipFile(filename);
    if (! zipFile.open(QIODevice::WriteOnly)) {
        return false;
    }
    zipFile.close();
    ZipWriter writer(zipFile.fileName());
    QStringList files = listIconFiles();
    files << "LICENSE"
          << "LICENSE.md"
          << "LICENSE.txt"
          << "README"
          << "README.md"
          << "README.txt"
          << "COPYING"
          << "mapping.json";
    for (QString file : files) {
        QFile sourceFile(toFilepath(file));
        if (sourceFile.open(QIODevice::ReadOnly)) {
            writer.addFile(file, sourceFile.readAll());
            sourceFile.close();
        }
    }
    writer.close();
    return true;
}


bool
IconManager::importBundle
(const QString &filename)
{
    QFile zipFile(filename);
    return importBundle(&zipFile);
}


bool
IconManager::importBundle
(const QUrl &url)
{
    QByteArray zipData = downloadUrl(url);
    QBuffer buffer;
    buffer.setData(zipData);
    buffer.open(QIODevice::ReadOnly);

    return importBundle(&buffer);
}


bool
IconManager::importBundle
(QIODevice *device)
{
    ZipReader reader(device);
    for (ZipReader::FileInfo info : reader.fileInfoList()) {
        if (info.isFile) {
            QByteArray data = reader.fileData(info.filePath);
            QFile file(baseDir.absoluteFilePath(info.filePath));
            QFileInfo fileInfo(file);
            if (! file.open(QIODevice::WriteOnly)) {
                continue;
            }
            qint64 bytesWritten = file.write(data);
            if (bytesWritten == -1) {
                continue;
            }
            file.close();
        }
    }
    reader.close();
    return loadMapping();
}


bool
IconManager::saveConfig
() const
{
    QJsonObject rootObj;
    for (const QString &field : icons.keys()) {
        writeGroup(rootObj, field, icons[field]);
    }

    QJsonDocument jsonDoc(rootObj);

    QFile file(baseDir.absoluteFilePath(configFile));
    if (! file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Could not open file for writing:" << file.errorString();
        return false;
    }
    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();

    return true;
}


bool
IconManager::loadMapping
()
{
    QFile file(baseDir.absoluteFilePath(configFile));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QByteArray jsonData = file.readAll();
        file.close();

        QJsonParseError parseError;
        QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);

        if (   ! jsonDoc.isNull()
            && jsonDoc.isObject()) {
            QJsonObject root = jsonDoc.object();
            icons["Sport"] = readGroup(root, "Sport");
            icons["SubSport"] = readGroup(root, "SubSport");
        }
    } else {
        qWarning().noquote().nospace()
            << "Cannot read icon mappings ("
            << file.fileName()
            << "): "
            << file.errorString();
        return false;
    }
    return true;
}


void
IconManager::writeGroup
(QJsonObject &rootObj, const QString &group, const QHash<QString, QString> &data) const
{
    QJsonObject groupObj;
    if (data.size() > 0) {
        for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
            groupObj.insert(it.key(), it.value());
        }
        rootObj.insert(group, groupObj);
    }
}


QHash<QString, QString>
IconManager::readGroup
(const QJsonObject &root, const QString &group)
{
    QHash<QString, QString> result;
    if (root.contains(group)) {
        QJsonObject groupObj = root.value(group).toObject();
        for (auto it = groupObj.constBegin(); it != groupObj.constEnd(); ++it) {
            QString filename = it.value().toString();
            if (filename.endsWith(".svg") && QFile::exists(toFilepath(filename))) {
                result.insert(it.key(), filename);
            }
        }
    }
    return result;
}


QByteArray
IconManager::downloadUrl
(const QUrl &url, int timeoutMs)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(url);
    QNetworkReply *reply = manager.get(request);

    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, [&]() {
        reply->abort();
        loop.quit();
    });
    timeoutTimer.start(timeoutMs);
    loop.exec();
    timeoutTimer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        delete reply;
        return {};
    }

    QByteArray data = reply->readAll();
    delete reply;
    return data;
}
