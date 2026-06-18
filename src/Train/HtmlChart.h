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

#ifndef _GC_HtmlChart_h
#define _GC_HtmlChart_h

#include "GoldenCheetah.h"

class QWebEngineView;
class QWebChannel;

class QTableWidget;
class QCheckBox;
class QTextEdit;
class QSplitter;
class QWidget;
class QTimer;

class Context;
class HtmlChart;

// This is the object that is exposed to the JavaScript environment of the chart, instead
// of the full HtmlChart object
class HtmlChartBridge : public QObject
{
    Q_OBJECT
public:
    explicit HtmlChartBridge(HtmlChart *chart, QObject *parent = nullptr);
    Q_INVOKABLE QString getChartConfig() const;

private:
    HtmlChart *m_chart;
};

class HtmlChart : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    // Properties for saving state in train-perspectives.xml
    Q_PROPERTY(QString html READ getHtml WRITE setHtml USER true)
    Q_PROPERTY(QByteArray splitterState READ getSplitterState WRITE setSplitterState USER true)
    Q_PROPERTY(bool showEditor READ isEditorVisible WRITE setEditorVisible USER true)
    Q_PROPERTY(QString chartConfig READ getChartConfigString WRITE setChartConfigString USER true)
    Q_PROPERTY(bool showTitleBar READ isTitleVisible WRITE setTitleVisible USER true)

public:
    HtmlChart(Context *context);
    ~HtmlChart();

    QString getHtml() const;
    void setHtml(const QString &text);

    QByteArray getSplitterState() const;
    void setSplitterState(const QByteArray &state);

    bool isEditorVisible() const;
    void setEditorVisible(bool visible);

    bool isTitleVisible() const;
    void setTitleVisible(bool visible);

    QString getChartConfig() const;
    QString getChartConfigString() const;
    void setChartConfigString(const QString &config);

protected:
    bool event(QEvent *e) override;

public slots:
    void applyHtml();
    void configChanged(qint32);
    void showEditorChanged(int state);
    void showConfigChanged(int state);
    void showTitleChanged(int state);
    void addConfigRow();
    void removeConfigRow();
    void configTableChanged();

private:
    Context *context;
    QSplitter *splitter;
    QTextEdit *editor;
    QCheckBox *showEditorBtn;
    QCheckBox *showConfigBtn;
    QCheckBox *showTitleBtn;
    QWidget *configWidget;
    QTableWidget *configTable;
    QWebEngineView *canvas;
    QWebChannel *m_webChannel;
    int m_savedTopMargin;
    QString currentHtml;
    QString currentChartConfig;
    QTimer *m_renderTimer;
};

#endif
