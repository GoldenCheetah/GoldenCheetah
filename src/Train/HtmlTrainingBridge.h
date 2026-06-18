/*
 * Copyright (c) 2026 GoldenCheetah
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

#ifndef _GC_HtmlTrainingBridge_h
#define _GC_HtmlTrainingBridge_h

#include <QObject>
#include <QPointF>
#include <QTimer>
#include <QString>

class Context;
class RealtimeData;
class ErgFile;

/**
 * HtmlTrainingBridge - Qt/JavaScript bridge for HTML training pages
 *
 * Exposes telemetry, configuration and route data to HTML pages via QtWebChannel.
 * Accessible in JavaScript as: window.gc.getConfig(), gc.telemetry.connect(...)
 */
class HtmlTrainingBridge : public QObject
{
    Q_OBJECT

public:
    explicit HtmlTrainingBridge(Context *context, QObject *parent = nullptr);
    ~HtmlTrainingBridge();

    /// Get configuration
    Q_INVOKABLE QString getConfig();

    /// Get planned route as GeoJSON
    Q_INVOKABLE QString getPlannedRoute();

signals:
    /// Emitted ~1Hz with telemetry data
    void telemetry(const QString &json);

    /// Emitted when erg file changes
    void plannedRouteChanged(const QString &json);
    /// Emitted when starts, stops, pauses, resumes
    void stateChanged(const QString &state);

private slots:
    void onTelemetryUpdate(const RealtimeData &rt);
    void onErgFileSelected(ErgFile *file);
    void onTelemetryThrottle();

    void onStop();
    void onStart();
    void onPause();
    void onResume();


private:
    QString formatTimestamp(qint64 msecs) const;

    Context *m_context;
    QString m_plannedRoute;
    QString m_currentTelemetry;
    QTimer *m_telemetryThrottle;
    bool m_hasPendingTelemetry;
    QString m_pendingTelemetrySample;
};

#endif