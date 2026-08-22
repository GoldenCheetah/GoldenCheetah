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

#include "HtmlChart.h"
#include "Context.h"
#include "HtmlTrainingBridge.h"
#include "Colors.h"
#include "AbstractView.h"
#include "HelpWhatsThis.h"

#include <QDebug>
#include <QTimer>

#include <QVBoxLayout>
#include <QWebEngineSettings>
#include <QWebEngineProfile>
#include <QApplication>
#include <QStyle>
#include <QWebEngineView>
#include <QWebChannel>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>

extern QApplication *application;

HtmlChartBridge::HtmlChartBridge(HtmlChart *chart, QObject *parent)
    : QObject(parent), m_chart(chart)
{
}

QString HtmlChartBridge::getChartConfig() const
{
    if (m_chart) return m_chart->getChartConfig();
    return "{}";
}

HtmlChart::HtmlChart(Context *context) : GcChartWindow(context), context(context), m_webChannel(nullptr)
{
    m_savedTopMargin = 0;

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    connect(m_renderTimer, &QTimer::timeout, this, &HtmlChart::applyHtml);

    QWidget *c = new QWidget;
    setControls(c);

    QVBoxLayout *clv = new QVBoxLayout(c);
    showEditorBtn = new QCheckBox(tr("Show Editor"), this);
    showEditorBtn->setChecked(true);
    clv->addWidget(showEditorBtn);

    showConfigBtn = new QCheckBox(tr("Show Config"), this);
    showConfigBtn->setChecked(false);
    clv->addWidget(showConfigBtn);

    showTitleBtn = new QCheckBox(tr("Show Title"), this);
    showTitleBtn->setChecked(true);
    clv->addWidget(showTitleBtn);

    clv->addStretch();

    connect(showEditorBtn, SIGNAL(stateChanged(int)), this, SLOT(showEditorChanged(int)));
    connect(showConfigBtn, SIGNAL(stateChanged(int)), this, SLOT(showConfigChanged(int)));
    connect(showTitleBtn, SIGNAL(stateChanged(int)), this, SLOT(showTitleChanged(int)));
    connect(context, SIGNAL(ergFileSelected(ErgFile*)), this, SLOT(applyHtml()));
    connect(context, SIGNAL(filterChanged()), this, SLOT(applyHtml()));
    connect(context, SIGNAL(homeFilterChanged()), this, SLOT(applyHtml()));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(2, 2, 2, 2);
    setChartLayout(mainLayout);

    splitter = new QSplitter(Qt::Vertical, this);
    splitter->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->setHandleWidth(1);
    mainLayout->addWidget(splitter);

    // Editor
    editor = new QTextEdit(this);
    editor->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    editor->setFrameStyle(QFrame::NoFrame);
    editor->setAcceptRichText(false);
    QFont courier("Courier", QFont().pointSize());
    editor->setFont(courier);

    connect(editor, &QTextEdit::textChanged, this, [this]() {
        if (m_renderTimer) m_renderTimer->start(1000);
    });

    splitter->addWidget(editor);

    // Config Widget
    configWidget = new QWidget(this);
    QVBoxLayout *configLayout = new QVBoxLayout(configWidget);
    configLayout->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *configBtnLayout = new QHBoxLayout;
    QPushButton *addBtn = new QPushButton(tr("Add"), this);
    QPushButton *removeBtn = new QPushButton(tr("Remove"), this);
    configBtnLayout->addWidget(addBtn);
    configBtnLayout->addWidget(removeBtn);
    configBtnLayout->addStretch();
    configLayout->addLayout(configBtnLayout);

    configTable = new QTableWidget(0, 2, this);
    configTable->setHorizontalHeaderLabels(QStringList() << tr("Key") << tr("Value"));
    configTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    configLayout->addWidget(configTable);

    splitter->addWidget(configWidget);

    connect(addBtn, SIGNAL(clicked()), this, SLOT(addConfigRow()));
    connect(removeBtn, SIGNAL(clicked()), this, SLOT(removeConfigRow()));
    connect(configTable, SIGNAL(itemChanged(QTableWidgetItem*)), this, SLOT(configTableChanged()));

    // Viewer
    canvas = new QWebEngineView(this);
    canvas->settings()->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    canvas->setPage(new QWebEnginePage(context->webEngineProfile, canvas));

    try {
        m_webChannel = new QWebChannel(this);
        HtmlTrainingBridge *bridge = context->getHtmlTrainingBridge();
        if (bridge) {
            m_webChannel->registerObject("gc", bridge);
        }

        HtmlChartBridge *chartBridge = new HtmlChartBridge(this, this);
        m_webChannel->registerObject("chart", chartBridge);

        canvas->page()->setWebChannel(m_webChannel);
        canvas->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    } catch (const std::exception &e) {
        qWarning() << "HTML Chart: Exception setting up web channel:" << e.what();
    }

    canvas->setContentsMargins(0, 0, 0, 0);
    canvas->setZoomFactor(dpiXFactor);
    canvas->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    splitter->addWidget(canvas);

    QList<int> sizes;
    sizes << 200 << 100 << 600;
    splitter->setSizes(sizes);

    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));
    configChanged(CONFIG_APPEARANCE);

    // Default HTML
    setHtml(R"HTML(<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
    <style>
        body { font-family: sans-serif; padding: 12px; color: #333; }
        #telemetry-data { display: grid; grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap: 8px; margin-top: 16px; }
        #telemetry-data div { padding: 8px; background: #f4f6f8; border-radius: 6px; border: 1px solid #d2d6dc; word-wrap: break-word; }
    </style>
</head>
<body>
    <h2>GC WebChannel</h2>
    <h3>Workout: <b id="workout-name"></b> (<b id="workout-type"></b>)</h3>
    <h4><b id="workout-description"></b></h4>
    <h4>Dark: <b id="dark"></b></h4>
    <div>Filename: <b id="workout-filename"></b></div>
    <div>Orig. Filename: <b id="workout-origfilename"></b></div>
    <div>Duration: <b id="workout-duration">0</b> secs.</div>
    <div>Tags: <b id="workout-tags"></b></div>
    <h3>Telemetry</h3>
    <div id="telemetry-data"></div>
    <div>State: <b id="state">Stopped</b></div>
    <script>
        if (typeof QWebChannel !== 'undefined') {
            new QWebChannel(qt.webChannelTransport, function(channel) {
                window.gc = channel.objects.gc;

                if (window.gc.getPlannedRoute) {
                    window.gc.getPlannedRoute(function(route) {
                        if (route) {
                            var d = typeof route === 'string' ? JSON.parse(route) : route;
                            document.getElementById("workout-name").innerText = d.workout.name;
                            document.getElementById("workout-description").innerText = d.workout.description;
                            document.getElementById("workout-type").innerText = d.workout.type;
                            document.getElementById("workout-filename").innerText = d.workout.filename;
                            document.getElementById("workout-origfilename").innerText = d.workout.originalFilename;
                            document.getElementById("workout-duration").innerText = (d.workout.duration_msecs / 1000 || 0).toFixed(1);;
                            document.getElementById("workout-tags").innerText = d.workout.tags;
                        }
                    });
                }

                if (window.gc.getConfig) {
                    window.gc.getConfig(function(config) {
                        if (config) {
                            var d = typeof config === 'string' ? JSON.parse(config) : config;
                            document.getElementById("dark").innerText = d.is_dark_background;
                        }
                    });
                }

                if (window.gc.telemetry) {
                    var telContainer = document.getElementById("telemetry-data");
                    window.gc.telemetry.connect(function(payload) {
                        var d = typeof payload === 'string' ? JSON.parse(payload) : payload;

                        for (var key in d) {
                            if (d.hasOwnProperty(key)) {
                                var safeId = key.replace(/[^A-Za-z0-9_-]/g, "");
                                var el = document.getElementById("tel-" + safeId);
                                if (!el) {
                                    var div = document.createElement("div");
                                    var label = document.createTextNode(key + ': ');
                                    div.appendChild(label);
                                    el = document.createElement("b");
                                    el.id = "tel-" + safeId;
                                    div.appendChild(el);
                                    telContainer.appendChild(div);
                                }

                                var val = d[key];
                                if (typeof val === 'number') {
                                    // Generic formatting for numeric values
                                    if (key.toLowerCase().includes('msec')) {
                                        el.innerText = (val / 1000).toFixed(1) + " s";
                                    } else {
                                        el.innerText = Number.isInteger(val) ? val : val.toFixed(2);
                                    }
                                } else {
                                    el.innerText = val;
                                }
                            }
                        }
                    });
                }
                if (window.gc.stateChanged) {
                    window.gc.stateChanged.connect(function(state) {
                        document.getElementById("state").innerText = state.toUpperCase();
                    });
                }
            });
        }
    </script>
</body>
</html>)HTML");

    showEditorChanged(showEditorBtn->checkState());
    showConfigChanged(showConfigBtn->checkState());
}

HtmlChart::~HtmlChart()
{
}

QString HtmlChart::getHtml() const
{
    if (editor) return editor->toPlainText();
    return currentHtml;
}

void HtmlChart::setHtml(const QString &text)
{
    if (editor) editor->setPlainText(text);
    currentHtml = text;
    applyHtml();
}

QByteArray HtmlChart::getSplitterState() const
{
    if (splitter) return splitter->saveState();
    return QByteArray();
}

void HtmlChart::setSplitterState(const QByteArray &state)
{
    if (splitter && !state.isEmpty()) splitter->restoreState(state);
}

void HtmlChart::applyHtml()
{
    if (editor && canvas) {
        currentHtml = editor->toPlainText();

        // Setting as html title the chart title (if set) for better identification in debug browser
        QString chartTitle = title();
        if (chartTitle.isEmpty()) chartTitle = "Html Chart";
        chartTitle.replace("\\", "\\\\").replace("'", "\\'").replace("\"", "\\\"").replace("<", "\\x3C").replace(">", "\\x3E");
        QString injectScript = QString("<script>document.title = '%1';</script>").arg(chartTitle);

        // Base url QUrl("qrc:/") is needed for the webchannel script
        canvas->page()->setHtml(currentHtml + injectScript, QUrl("qrc:/"));
    }
}

void HtmlChart::configChanged(qint32)
{
    QColor bgcolor = GColor(CTRAINPLOTBACKGROUND);
    setProperty("color", bgcolor);

    QPalette palette;
    palette.setBrush(QPalette::Window, QBrush(GColor(CPLOTBACKGROUND)));
    palette.setColor(QPalette::WindowText, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Text, GColor(CPLOTMARKER));
    palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
    setPalette(palette);

    if (editor) {
        editor->setPalette(palette);
        editor->setStyleSheet(AbstractView::ourStyleSheet());
    }
}


bool HtmlChart::isTitleVisible() const
{
    return showTitleBtn->isChecked();
}

void HtmlChart::setTitleVisible(bool visible)
{
    showTitleBtn->blockSignals(true);
    showTitleBtn->setChecked(visible);
    showTitleBtn->blockSignals(false);

    GcChartWindow::setShowTitle(visible);

    if (visible) {
        if (m_savedTopMargin == 0) m_savedTopMargin = 15; // fallback
        setContentsMargins(0, m_savedTopMargin, 0, 0);
    } else {
        if (contentsMargins().top() > 0) {
            m_savedTopMargin = contentsMargins().top();
        }
        setContentsMargins(0, 0, 0, 0);
    }
    update();
}

bool HtmlChart::event(QEvent *e)
{
    if (e->type() == QEvent::ContentsRectChange || e->type() == QEvent::Resize || e->type() == QEvent::Show || e->type() == QEvent::LayoutRequest) {
        if (showTitleBtn && !showTitleBtn->isChecked() && contentsMargins().top() > 0) {
            m_savedTopMargin = contentsMargins().top();
            setContentsMargins(0, 0, 0, 0);
        }
    }
    return GcChartWindow::event(e);
}

void HtmlChart::showTitleChanged(int state)
{
    setTitleVisible(state == Qt::Checked);
}

void HtmlChart::showEditorChanged(int state)
{
    if (editor) editor->setVisible(state);
}

bool HtmlChart::isEditorVisible() const
{
    return showEditorBtn ? showEditorBtn->isChecked() : false;
}

void HtmlChart::setEditorVisible(bool visible)
{
    if (showEditorBtn) {
        showEditorBtn->setChecked(visible);
    }
}

void HtmlChart::showConfigChanged(int state)
{
    if (configWidget) configWidget->setVisible(state);
}

QString HtmlChart::getChartConfig() const
{
    return currentChartConfig.isEmpty() ? "{}" : currentChartConfig;
}

QString HtmlChart::getChartConfigString() const
{
    return currentChartConfig;
}

void HtmlChart::setChartConfigString(const QString &config)
{
    currentChartConfig = config;

    if (configTable) {
        configTable->blockSignals(true);
        configTable->setRowCount(0);
        if (!config.isEmpty()) {
            QJsonDocument doc = QJsonDocument::fromJson(config.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
                    int row = configTable->rowCount();
                    configTable->insertRow(row);
                    configTable->setItem(row, 0, new QTableWidgetItem(it.key()));
                    configTable->setItem(row, 1, new QTableWidgetItem(it.value().toVariant().toString()));
                }
            }
        }
        configTable->blockSignals(false);
    }
}

void HtmlChart::addConfigRow()
{
    if (!configTable) return;
    configTable->blockSignals(true);
    int row = configTable->rowCount();
    configTable->insertRow(row);
    configTable->setItem(row, 0, new QTableWidgetItem("new_key"));
    configTable->setItem(row, 1, new QTableWidgetItem("value"));
    configTable->blockSignals(false);
    configTableChanged();
}

void HtmlChart::removeConfigRow()
{
    if (!configTable) return;
    int row = configTable->currentRow();
    if (row >= 0) {
        configTable->removeRow(row);
        configTableChanged();
    }
}

void HtmlChart::configTableChanged()
{
    if (!configTable) return;
    QJsonObject obj;
    for (int i = 0; i < configTable->rowCount(); ++i) {
        QTableWidgetItem *keyItem = configTable->item(i, 0);
        QTableWidgetItem *valItem = configTable->item(i, 1);
        if (keyItem && valItem) {
            QString key = keyItem->text().trimmed();
            if (!key.isEmpty()) {
                obj[key] = valItem->text();
            }
        }
    }
    currentChartConfig = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));

    applyHtml();
}
