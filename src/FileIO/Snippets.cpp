/*
 * Copyright (c) 2016 Mark Liversedge (liversedge@gmail.com)
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

#include "DataProcessor.h"
#include "Athlete.h"
#include "Context.h"
#include "Settings.h"
#include "Units.h"
#include "IntervalItem.h"
#include "HelpWhatsThis.h"
#include <algorithm>
#include <QVector>

// Config widget used by the Preferences/Options config panes
class Snippets;
class SnippetsConfig : public DataProcessorConfig
{
    Q_DECLARE_TR_FUNCTIONS(SnippetsConfig)

    friend class ::Snippets;
    protected:

    public:
        SnippetsConfig(QWidget *parent) : DataProcessorConfig(parent) {

            //HelpWhatsThis *help = new HelpWhatsThis(parent);
            //parent->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::MenuBar_Edit_SnippetsInRecording));
        }

        void readConfig() {
        }

        void saveConfig() {
        }
};


// RideFile Dataprocessor -- dumps a journal file of metrics to the snippets folder
//                           useful to record metric changes as data is edited
//                           typically used by external databases
//
class Snippets : public DataProcessor {
    Q_DECLARE_TR_FUNCTIONS(Snippets)

    public:
        Snippets() {}
        ~Snippets() {}

        // the processor
        bool postProcess(RideFile *, DataProcessorConfig* config, QString op) override;

        // the config widget
        DataProcessorConfig* processorConfig(QWidget *parent, const RideFile * ride = NULL) const override {
            Q_UNUSED(ride);
            return new SnippetsConfig(parent);
        }

        // Localized Name
        QString name() const override {
            return tr("Snippet export");
        }

        QString id() const override {
            return "::Snippets";
        }

        QString legacyId() const override {
            return "Snippet export";
        }

        QString explain() const override {
            return tr("Dump metrics for the ride to Athlete_Home/Snippets");
        }
};

static bool SnippetsAdded = DataProcessorFactory::instance().registerProcessor(new Snippets());

// how we write the ride time
#define DATETIME_FORMAT "yyyy/MM/dd hh:mm:ss' UTC'"

// Escape special characters (JSON compliance)
static QString protect(const QString string)
{
    QString s = string;
    s.replace("\\", "\\\\"); // backslash
    s.replace("\"", "\\\""); // quote
    s.replace("\t", "\\t");  // tab
    s.replace("\n", "\\n");  // newline
    s.replace("\r", "\\r");  // carriage-return
    s.replace("\b", "\\b");  // backspace
    s.replace("\f", "\\f");  // formfeed
    s.replace("/", "\\/");   // solidus

    // add a trailing space to avoid conflicting with GC special tokens
    s += " ";

    return s;
}

bool
Snippets::postProcess(RideFile *ride, DataProcessorConfig *config=0, QString op="")
{
    Q_UNUSED(config);

    // create a snippet file if its not null
    if (!ride) return false;

    // whats the next counter?
    qint64 journalid = appsettings->cvalue(ride->context->athlete->cyclist,  GC_ATHLETE_SNIPPETID, 0).toLongLong();
    journalid ++;
    QString filename = ride->context->athlete->home->snippets().absolutePath() + QString("/%1.json").arg(journalid, 9, 10, QChar('0'));
    QFile outfile(filename);

    // open the file, fails are graceful but silent
    if (outfile.open(QIODevice::WriteOnly)) {

        // setup streamer
        QTextStream out(&outfile);

        // unified codepage and BOM for identification on all platforms
        //out.setGenerateByteOrderMark(true); << make it easier to parse with no BOM

        out << "{\n\t\"" << op << "\": {\n";

        //
        // FIRST CLASS MEMBERS (IDENTIFIERS etc)
        //
        out << "\t\t\"STARTTIME\": \"" << protect(ride->startTime().toUTC().toString(DATETIME_FORMAT)) << "\",\n";
        out << "\t\t\"RECINTSECS\": " << ride->recIntSecs() << ",\n";
        out << "\t\t\"DEVICETYPE\": \"" << protect(ride->deviceType()) << "\",\n";
        out << "\t\t\"IDENTIFIER\": \"" << protect(ride->id()) << "\"";

        //
        // TAGS
        //
        if (ride->tags().count()) {

            out << ",\n\t\t\"TAGS\": {\n";

            QMap<QString,QString>::const_iterator i;
            for (i=ride->tags().constBegin(); i != ride->tags().constEnd(); i++) {

                    out << "\t\t\t\"" << i.key() << "\": \"" << protect(i.value()) << "\"";
                    if (std::next(i) != ride->tags().constEnd()) out << ",\n";
                    else out << "\n";
            }

            // end of the tags
            out << "\t\t}";
        }

        //
        // RIDE METRICS
        //

        out << ",\n\t\t\"METRICS\": {\n";

        // calculate metrics - we need to do this ourselves as we are working
        // with ride data, not a rideitem, so we manufacture a rideitem too. sigh.
        RideItem rideItem(ride, ride->context);
        const RideMetricFactory &factory = RideMetricFactory::instance();
        QHash<QString,RideMetricPtr> computed= RideMetric::computeMetrics(&rideItem, Specification(), factory.allMetrics());

        // write them out
        bool first = true;
        QHashIterator<QString, RideMetricPtr> i(computed);
        while (i.hasNext()) {
            i.next();
            double v =i.value()->value();

            // clean bad values - we always write all metrics
            if (std::isinf(v) || std::isnan(v)) v = 0;

            if (!first) out << ",\n";
            out << "\t\t\t\"" << i.value()->name() << "\": \"" << QString("%1").arg(v) << "\"";

            first = false;
        }
        out << "\n\t\t}";

        //
        // INTERVALS
        //
        if (ride->intervals().count()) {

            out << ",\n\t\t\"INTERVALS\": [";

            bool first = true;
            // and output each one
            foreach(RideFileInterval *ri, ride->intervals()) {

                // create an interval item for each interval
                IntervalItem interval(&rideItem, ri->name, ri->start, ri->stop, 0, 0, 1,
                                             QColor(Qt::black), ri->test, RideFileInterval::USER);
                // refresh metrics
                interval.refresh();

                // lists of intervals
                if (first) out << "\n";
                else out << ",\n";
                first=false;

                // name is all we put in there
                out << "\t\t\t{\n";
                out << "\t\t\t\t\"name\": \"" << protect(ri->name) << "\",\n";

                // the metrics for this interval
                out << "\t\t\t\t\"METRICS\": {\n";

                //
                // INTERVAL METRICS
                //
                bool firstMetric = true;
                for(int i=0; i<factory.metricCount(); i++) {
                    QString sym = factory.metricName(i);
                    const RideMetric *m = factory.rideMetric(sym);
                    QString name = m ? m->name() : sym;
                    int index = factory.rideMetric(sym)->index();
                    if (!firstMetric) out << ",\n";
                    firstMetric = false;
                    out << "\t\t\t\t\t\"" << name << "\":\"" << QString("%1").arg(interval.metrics()[index], 0, 'f', 5) <<"\"";
                }

                out << "\n\t\t\t\t}\n";

                // and we're done
                out << "\t\t\t}";
            }

            out << "\n\t\t]";
        }

        // all done
        out << "\n\t}\n}\n";

        // before rideitem goes out of scope set ride to NULL
        // so it doesn't get deleted !
        rideItem.setRide(NULL);

        // save the last journalid
        appsettings->setCValue(ride->context->athlete->cyclist,  GC_ATHLETE_SNIPPETID, journalid);

        // close the file
        outfile.close();
    }
    return true;
}
