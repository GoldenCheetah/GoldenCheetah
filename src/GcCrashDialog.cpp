/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "GcCrashDialog.h"
#include "Settings.h"
#include <QtGui>
#include <QWebFrame>

GcCrashDialog::GcCrashDialog(QDir home) : QDialog(NULL, Qt::Dialog), home(home)
{
    setAttribute(Qt::WA_DeleteOnClose, true); // caller must delete me, once they've extracted the name
    setWindowTitle(QString(tr("%1 Crash Recovery").arg(home.dirName())));

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *toprow = new QHBoxLayout;

    QPushButton *critical = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxCritical), "", this); 
    critical->setFixedSize(128,128);
    critical->setFlat(true);
    critical->setIconSize(QSize(120,120));
    critical->setAutoFillBackground(false);
    critical->setFocusPolicy(Qt::NoFocus);

    QLabel *header = new QLabel(this);
    header->setWordWrap(true);
    header->setTextFormat(Qt::RichText);
    header->setText(tr("<b>GoldenCheetah appears to have PREVIOUSLY crashed for this athlete. "
                       "</b><br><br>The report below gives some diagnostic information "
                       "that will be useful to the developers. Feel free to post this with a "
                       "short description of what was occurring when the crash happened to the "
                       "<href a=\"https://groups.google.com/forum/?fromgroups=#!forum/golden-cheetah-users\">"
                       "GoldenCheetah forums</href><br>"
                       "<b><br>We respect privacy - this log does NOT contain ids, passwords or personal information.</b><br>"
                       "<b><br>When this dialog is closed the athlete will be opened.</b>"));

    toprow->addWidget(critical);
    toprow->addWidget(header);
    layout->addLayout(toprow);

    report = new QWebView(this);
    report->setContentsMargins(0,0,0,0);
    report->page()->view()->setContentsMargins(0,0,0,0);
    report->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    report->setAcceptDrops(false);

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply
    report->settings()->setFontSize(QWebSettings::DefaultFontSize, defaultFont.pointSize()+1);
    report->settings()->setFontFamily(QWebSettings::StandardFont, defaultFont.family());

    layout->addWidget(report);

    QHBoxLayout *lastRow = new QHBoxLayout;
    QPushButton *saveAsButton = new QPushButton(tr("Save Crash Report..."), this);
    connect(saveAsButton, SIGNAL(clicked()), this, SLOT(saveAs()));
    QPushButton *OKButton = new QPushButton(tr("Close"), this);
    connect(OKButton, SIGNAL(clicked()), this, SLOT(accept()));

    lastRow->addWidget(saveAsButton);
    lastRow->addStretch();
    lastRow->addWidget(OKButton);
    layout->addLayout(lastRow);

    setHTML();
}

void
GcCrashDialog::setHTML()
{
    QString text;

    // the cyclist...
    text += QString("<center><h3>Cyclist: \"%1\"</h3></center><br>").arg(home.dirName());

    // metric log...
    text += "<center><h3>Metric Log</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    QFile metriclog(home.absolutePath() + "/" + "metric.log");
    if (metriclog.open(QIODevice::ReadOnly)) {

        // read in line by line and add to diag file
        QTextStream in(&metriclog);
        while (!in.atEnd()) {
            QString lines = in.readLine();
            foreach(QString line, lines.split('\r')) {
                text += "<tr><td align=\"center\">" + line + "</td></tr>";
            }
        }

        metriclog.close();
    }
    text += "</table></center>";

    // files...
    text += "<center><h3>Athlete Directory</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.entryList(QDir::NoFilter, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.absolutePath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";


    // settings...
    text += "<center><h3>All Settings</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString key, appsettings->allKeys()) {

        // RESPECT PRIVACY
        // we do not disclose user names and passwords
        if (key.endsWith("/user") || key.endsWith("/pass")) continue;

        // we do not disclose personally identifiable information
        if (key.endsWith("/dob") || key.endsWith("/weight") ||
            key.endsWith("/sex") || key.endsWith("/bio")) continue;


        text += QString("<tr><td align=\"right\" width=\"50%\"> %1</td><td align=\"left\">"
                        "<span style=\"max-width:50%;\">%2</span></td></tr>")
                .arg(key)
                .arg(appsettings->value(NULL, key).toString().leftJustified(60, ' ', true));
    }
    text += "</table></center>";


	report->page()->mainFrame()->setHtml(text);
}

void
GcCrashDialog::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName( this, tr("Save Diagnostics"), QDir::homePath(), tr("Text File (*.txt)"));

    // now write to it
    QFile file(fileName);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    if (file.open(QIODevice::WriteOnly)) {

        // write the texts
        out << report->page()->mainFrame()->toPlainText();

    }
    file.close();
}
