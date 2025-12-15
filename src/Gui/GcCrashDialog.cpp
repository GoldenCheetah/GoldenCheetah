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
#include "GcUpgrade.h"
#include <QtGui>
#include <QDateTime>
#include <QSslSocket>

#include "RideMetric.h"
#include <QtSql>
#include <qwt_plot_curve.h>

#ifdef Q_CC_GNU
#define COMPILER "GCC"
#define COMPILER_VERSION QString("%1.%2.%3").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__)
#endif

#ifdef Q_CC_MSVC
#define COMPILER "MSVC"
#define COMPILER_VERSION QString("%1").arg(_MSC_VER)
#endif

#include "VideoWindow.h"

#ifdef GC_HAVE_ICAL
#include "ICalendar.h"
#endif

#ifdef GC_HAVE_D2XX
#include "D2XX.h"
#endif

#ifdef GC_HAVE_SRMIO
#include "srmio.h"
#endif

#ifdef GC_HAVE_SAMPLERATE
#include <samplerate.h>
#endif

#ifdef GC_WANT_R
#include "RTool.h"
#include "Rversion.h"
#endif

#ifdef GC_WANT_PYTHON
#include "PythonEmbed.h"
#endif

#include <gsl/gsl_version.h>

QString gl_version;

GcCrashDialog::GcCrashDialog(QDir homeDir) : QDialog(NULL, Qt::Dialog), home(homeDir)
{
    setAttribute(Qt::WA_DeleteOnClose, true); // caller must delete me, once they've extracted the name
    setWindowTitle(QString(tr("%1 Crash Recovery").arg(home.root().dirName())));

    //check if the problem occured when adding to RideCache - by checking if there are files in /tmpActivities
    newFilesInQuarantine = false;
    files = home.tmpActivities().entryList(QDir::Files);
    if (!files.empty()) {
        // yes there are files which caused problems - so quarantine them
        newFilesInQuarantine = true;
        foreach (QString file, files) {
            // just try to move by renaming
            QString source = home.tmpActivities().canonicalPath() + "/" + file;
            // create a unique file name in case of multipe crashes with the same file
            QString target = home.quarantine().canonicalPath() + "/" + file;
            QFile s(source);
            QFile t(target);
            if (t.exists()) {
               // remove target first if target already exists in /quarantine - from previous crash - only keep the last crashed version
               t.remove();
            }
            s.rename(target);
        }
    }


    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *toprow = new QHBoxLayout;

    QPushButton *critical = new QPushButton(style()->standardIcon(QStyle::SP_MessageBoxCritical), "", this); 
    critical->setFixedSize(128*dpiXFactor,128*dpiYFactor);
    critical->setFlat(true);
    critical->setIconSize(QSize(120*dpiXFactor,120*dpiYFactor));
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

    QFont defaultFont; // mainwindow sets up the defaults.. we need to apply

    report = new QWebEngineView(this);
    report->setContentsMargins(0,0,0,0);
    report->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    report->setAcceptDrops(false);
    report->settings()->setFontSize(QWebEngineSettings::DefaultFontSize, defaultFont.pointSize()+1);
    report->settings()->setFontFamily(QWebEngineSettings::StandardFont, defaultFont.family());

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

GcCrashDialog::~GcCrashDialog()
{
    if (report) delete report->page();
}

QString GcCrashDialog::versionHTML()
{
    // -- OS ----
    QString os = QSysInfo::prettyProductName();

    // -- SCHEMA VERSION ----
    QString schemaVersion = QString("%1").arg(DBSchemaVersion);

    // -- SRMIO ----
    QString srmio = "none";

    #ifdef GC_HAVE_SRMIO
    #ifdef SRMIO_VERSION
    srmio = QString("%1 %2").arg(SRMIO_VERSION).arg(srmio_commit);
    #else
    srmio = "yes";
    #endif
    #endif

    // -- D2XX ----
    QString d2xx = "none";

    #ifdef GC_HAVE_D2XX
    d2xx = "yes";
    #endif

    // -- ICAL ----
    QString ical = "none";

    #ifdef GC_HAVE_ICAL
    ical = ICAL_VERSION;
    #endif

    // -- USBXPRESS ----
    QString usbxpress = "none";
    #ifdef GC_HAVE_USBXPRESS
    usbxpress = "yes";
    #endif

    // -- LIBUSB ----
    QString libusb = "none";
    #ifdef GC_HAVE_LIBUSB
    #ifdef LIBUSB_V_1
    libusb = "yes (1.0)";
    #else
    libusb = "yes";
    #endif
    #endif

    // -- VLC ----
    QString vlc = "none";
    #ifdef GC_HAVE_VLC
    vlc = libvlc_get_version();
    #endif

    #ifdef GC_HAVE_SAMPLERATE
    QString src = QString(src_get_version()).mid(14,6);
    #else
    QString src = "none";
    #endif

    // -- GSL --
    QString gsl = "none";
    gsl = GSL_VERSION;

    const RideMetricFactory &factory = RideMetricFactory::instance();
    QString gc_version = tr(
            "<p>Build date: %1 %2"
            "<br>Build id: %3"
            "<br>Version: %4"
            "<br>DB Schema: %5"
            "<br>Metrics: %7"
            "<br>OS: %6"
#ifdef Q_OS_LINUX
            "<br>OpenGL: %10"
#endif
            "<br>UI: dpi scale (%8) font size (%9)"
            "<br>")
            .arg(__DATE__)
            .arg(__TIME__)
            .arg(GcUpgrade::version())
#ifdef GC_VERSION
            .arg(GC_VERSION)
#else
            .arg("(developer build)")
#endif
            .arg(schemaVersion)
            .arg(os)
            .arg(factory.metricCount())
            .arg(dpiXFactor)
            .arg(QFont().pointSizeF())
#ifdef Q_OS_LINUX
            .arg(gl_version)
#endif
            ;

    QString lib_version = tr(
            "<table>"
            "<tr><td colspan=\"2\">QT</td><td>%1</td></tr>"
            "<tr><td colspan=\"2\">QWT</td><td>%2</td></tr>"
            "<tr><td colspan=\"2\">%3</td><td>%4</td></tr>"
            "<tr><td colspan=\"2\">SRMIO</td><td>%5</td></tr>"
            "<tr><td colspan=\"2\">D2XX</td><td>%6</td></tr>"
            "<tr><td colspan=\"2\">ICAL</td><td>%9</td></tr>"
            "<tr><td colspan=\"2\">USBXPRESS</td><td>%10</td></tr>"
            "<tr><td colspan=\"2\">LIBUSB</td><td>%11</td></tr>"
            "<tr><td colspan=\"2\">VLC</td><td>%12</td></tr>"
            "<tr><td colspan=\"2\">VIDEO</td><td>%13</td></tr>"
            "<tr><td colspan=\"2\">SAMPLERATE</td><td>%14</td></tr>"
            "<tr><td colspan=\"2\">SSL</td><td>%15</td></tr>"
            "<tr><td colspan=\"2\">R</td><td>%16</td></tr>"
            "<tr><td colspan=\"2\">Python</td><td>%18</td></tr>"
            "<tr><td colspan=\"2\">LMFIT</td><td>7.0</td></tr>"
            "<tr><td colspan=\"2\">GSL</td><td>%19</td></tr>"
            "</table>"
            )
            .arg(QT_VERSION_STR)
            .arg(QWT_VERSION_STR)
            .arg(COMPILER)
            .arg(COMPILER_VERSION)
            .arg(srmio)
            .arg(d2xx)
            .arg(ical)
            .arg(usbxpress)
            .arg(libusb)
            .arg(vlc)
#if defined GC_VIDEO_QT6
            .arg("qt6")
#elif defined GC_VIDEO_VLC
            .arg("vlc")
#else
            .arg("none")
#endif
            .arg(src)
            .arg(QSslSocket::supportsSsl() ? "yes" : "none")
#ifdef GC_WANT_R
            .arg(QString("%1 [%2.%3]").arg(rtool ? rtool->version : QString("none")).arg(R_MAJOR).arg(R_MINOR))
#else
            .arg("none")
#endif
#ifdef GC_HAVE_PYTHON
            .arg(QString("%1 [%2]").arg(python ? python->version.split(" ").at(0) : QString("none")).arg(PythonEmbed::buildVersion()))
#else
            .arg("none")
#endif
            .arg(gsl)
            ;

    QString versionText = QString("<center>"  + gc_version  + lib_version + "</center>");

    return versionText;
}

void
GcCrashDialog::setHTML()
{
    QString text;

    // the cyclist...
    text += QString("<center><h3>Cyclist: \"%1\"</h3></center><br>").arg(home.root().dirName());

    // version info
    text += "<center><h3>Version Info</h3></center>";
    text += versionHTML();

    // quarantine info
    if (newFilesInQuarantine) {
        text += "<center><h3>Quarantine Info</h3></center>";
        text += "The following file(s) created by 'Import from file' or 'Download from device' have been moved to subdirectory '/quarantine' "
                "since they most likely caused the crash - e.g. because of corrupt data - during the creation of the GoldenCheetah metric cache. "
                "Please check both the GoldenCheetah .JSON files and the associated source files." ;
        text += "<center><table border=0 cellspacing=10 width=\"90%\">";
        foreach (QString file, files) {
          text += "<tr><td align=\"center\">" + file + "</td></tr>";
        }
        text += "</table></center>";
    }

    // get the complete file list which are already in /quarantine
    QStringList quarantine = home.quarantine().entryList(QDir::Files);
    if (!quarantine.empty()) {
        text += "<br><h3><center>All Quarantined Files</h3></center>";
        text += "<center><table border=0 cellspacing=10 width=\"90%\">";
        foreach (QString file, quarantine) {
          text += "<tr><td align=\"center\">" + file + "</td></tr>";
        }
        text += "</table></center>";
    }

    // metric log...
    text += "<center><h3>Metric Log</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    QFile metriclog(home.logs().canonicalPath() + "/" + "metric.log");
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
    text += "<center><h3>Athlete Directory - Activities</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.activities().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.activities().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    text += "<center><h3>Athlete Directory - Cache</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.cache().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.cache().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    text += "<center><h3>Athlete Directory - Config</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.config().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.config().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    text += "<center><h3>Athlete Directory - Workouts</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.workouts().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.workouts().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    text += "<center><h3>Athlete Directory - Imports</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.imports().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.imports().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    text += "<center><h3>Athlete Directory - Downloads</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString file, home.downloads().entryList(QDir::NoDotAndDotDot, QDir::Time)) {
        text += QString("<tr><td align=\"right\"> %1</td><td align=\"left\">%2</td></tr>")
                .arg(file)
                .arg(QFileInfo(home.downloads().canonicalPath() + "/" + file).lastModified().toString());
    }
    text += "</table></center>";

    // settings...
    text += "<center><h3>All Settings</h3></center>";
    text += "<center><table border=0 cellspacing=10 width=\"90%\">";
    foreach(QString key, appsettings->allKeys()) {

        // RESPECT PRIVACY
        // we do not disclose user names and passwords or key ids
        if (key.endsWith("/user") || key.endsWith("/pass") ||
            key.endsWith("/key") || key.endsWith("token") ||
            key.endsWith("_secret") || key.endsWith("googlecalid")) continue;

        // we do not disclose personally identifiable information
        if (key.endsWith("dob") || key.endsWith("weight") ||
            key.endsWith("sex") || key.endsWith("bio") ||
            key.endsWith("height") || key.endsWith("nickname")) continue;

        if (key.startsWith("<athlete-")) {
            text += QString("<tr><td align=\"right\" width=\"50%\"> %1</td><td align=\"left\">"
                            "<span style=\"max-width:50%;\">%2</span></td></tr>")
                    .arg(key)
                    .arg(appsettings->cvalue(home.root().dirName(), key).toString().leftJustified(60, ' ', true));
        } else {
            text += QString("<tr><td align=\"right\" width=\"50%\"> %1</td><td align=\"left\">"
                            "<span style=\"max-width:50%;\">%2</span></td></tr>")
                    .arg(key)
                    .arg(appsettings->value(NULL, key).toString().leftJustified(60, ' ', true));
        }
    }
    text += "</table></center>";

    report->page()->setHtml(text);
}

void
GcCrashDialog::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName( this, tr("Save Diagnostics"), QDir::homePath(), tr("Text File (*.txt)"));

    // now write to it
    QFile file(fileName);
    file.resize(0);
    QTextStream out(&file);

    if (file.open(QIODevice::WriteOnly)) {
        // write the texts
        file.close();
    }
}
