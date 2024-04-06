/*
 * Copyright (c) 2020 Mark Liversedge (liversedge@gmail.com)
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

#include "MetricSelect.h"
#include "RideFileCache.h"

MetricSelect::MetricSelect(QWidget *parent, Context *context, int scope)
    : QLineEdit(parent), context(context), scope(scope), _metric(NULL)
{

    // add metrics if we're selecting those
    if (scope & Metric) {
        const RideMetricFactory &factory = RideMetricFactory::instance();
        foreach(QString name, factory.allMetrics()) {
            const RideMetric *m = factory.rideMetric(name);
            QString text = m->internalName();
            text = text.replace(' ','_');
            if (text.startsWith("BikeScore")) text="BikeScore"; // stoopid tm sign in name ffs sean
            listText << text;
            metricMap.insert(text, m);
        }
    }

    // add meta if we're selecting this
    if (scope & Meta) {

        // now add the ride metadata fields -- should be the same generally
        foreach(FieldDefinition field, GlobalContext::context()->rideMetadata->getFields()) {
            QString text = field.name;
            if (!GlobalContext::context()->specialFields.isMetric(text)) {

                // translate to internal name if name has non Latin1 characters
                text = GlobalContext::context()->specialFields.internalName(text);
                text = text.replace(" ","_");
                metaMap.insert(text, field.name);
            }
            listText << text;
        }
    }

    // set the autocompleter, if there is something to autocomplete
    if (listText.count()) {
        completer = new QCompleter(listText, this);
        setCompleter(completer);
    }
}

// set text via metric symbol
void
MetricSelect::setSymbol(QString symbol)
{
    if ((scope & MetricSelect::Metric) == 0) return;

    // get the ridemetric
    RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *m = factory.rideMetric(symbol);

    // lets set as per normal...
    if (m) {

        QString text = m->internalName();
        text = text.replace(' ','_');
        if (text.startsWith("BikeScore")) text="BikeScore"; // stoopid tm sign in name ffs sean
        setText(text);
        setWhatsThis(m->description());
    }
    // check it...
    isValid();
}
// is the entry we have in there even valid?
bool
MetricSelect::isValid()
{
    _metric = NULL;
    if (scope & MetricSelect::Metric) {
        _metric = metricMap.value(text(), NULL);
        if (_metric) return true;
    }

    if (scope & MetricSelect::Meta) {
        if (metaMap.value(text(),"") != "") return true;
    }

    return false;
}

bool
MetricSelect::isMetric()
{
    isValid();
    return (_metric != NULL);
}

bool
MetricSelect::isMeta()
{
    if (!isMetric() && metaMap.contains(text())) return true;
    return false;
}

const RideMetric *
MetricSelect::rideMetric()
{
    isValid(); // check
    return _metric;
}

QString MetricSelect::metaname()
{
    isValid();
    if (_metric) return "";
    else return metaMap.value(text(), "");
}

void
MetricSelect::setMeta(QString name)
{
    if (_metric) return;
    else {
        // find it
        QMapIterator<QString,QString>it(metaMap);
        while(it.hasNext()) {
            it.next();
            if (it.value() == name) {
                setText(it.key());
                break;
            }
        }
    }
}

SeriesSelect::SeriesSelect(QWidget *parent, int scope) : QComboBox(parent), scope(scope)
{
    // set the scope
    QStringList symbols;
    if (scope == SeriesSelect::All)  {
        foreach(QString symbol, RideFile::symbols()) {
            if (symbol != "") symbols << symbol; // filter out dummy values
        }
    } else if (scope == SeriesSelect::MMP) {
        foreach(RideFile::SeriesType series, RideFileCache::meanMaxList()) {
            symbols << RideFile::symbolForSeries(series);
        }
    } else if (scope == SeriesSelect::Zones) {
        symbols << "POWER" << "HEARTRATE" << "SPEED" << "WBAL";
    }

    foreach(QString symbol, symbols) {
        addItem(symbol, static_cast<int>(RideFile::seriesForSymbol(symbol)));
    }
}

void
SeriesSelect::setSeries(RideFile::SeriesType x)
{
    // loop through values to determine which to select
    for(int i=0; i<count(); i++) {
        if (itemData(i, Qt::UserRole).toInt() == static_cast<int>(x)) {
            setCurrentIndex(i);
            return;
        }
    }
}

RideFile::SeriesType
SeriesSelect::series()
{
    if (currentIndex() < 0)  return RideFile::none;
    return static_cast<RideFile::SeriesType>(itemData(currentIndex(), Qt::UserRole).toInt());
}
