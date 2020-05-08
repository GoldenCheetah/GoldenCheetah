/*
 * Copyright (c) 2009 Mark Liversedge (liversedge@gmail.com)
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

#include "IntervalItem.h"
#include "Specification.h"
#include "RideFile.h"
#include "Context.h"
#include "Athlete.h"
#include "Colors.h"
#include "ColorButton.h"

IntervalItem::IntervalItem(const RideItem *ride, QString name, double start, double stop, 
                           double startKM, double stopKM, int displaySequence, QColor color, bool test,
                           RideFileInterval::IntervalType type)
{
    this->name = name;
    this->start = start;
    this->stop = stop;
    this->startKM = startKM;
    this->stopKM = stopKM;
    this->displaySequence = displaySequence;
    this->type = type;
    this->color = color;
    this->selected = false;
    this->test = test;
    this->rideInterval = NULL;
    this->rideItem_ = const_cast<RideItem*>(ride);
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

IntervalItem::IntervalItem() : rideItem_(NULL), selected(false), name(""), type(RideFileInterval::USER), start(0), stop(0),
                               startKM(0), stopKM(0), displaySequence(0), color(Qt::black), test(false), rideInterval(NULL)
{
    metrics_.fill(0, RideMetricFactory::instance().metricCount());
    count_.fill(0, RideMetricFactory::instance().metricCount());
}

void
IntervalItem::setFrom(IntervalItem &other)
{
    *this = other;
    rideItem_ = other.rideItem_;
    rideInterval = NULL;
    selected = other.selected;
}

void
IntervalItem::setValues(QString name, double duration1, double duration2, 
                                      double distance1, double distance2,
                                      QColor color, bool test)
{
    // apply the update
    this->name = name;
    this->test = test;
    this->color = color;
    start = duration1;
    stop = duration2;
    startKM = distance1;
    stopKM = distance2;

    // only accept changes if we can send on
    if (type == RideFileInterval::USER && rideInterval && rideItem_) {

        // update us and our ridefileinterval
        rideInterval->start = start = duration1;
        rideInterval->stop = stop = duration2;
        rideInterval->color = color;
        startKM = distance1;
        stopKM = distance2;

        // update ridefile
        rideItem_->setDirty(true);

        // update metrics
        refresh();
    }

}

void
IntervalItem::refresh()
{
    // metrics
    const RideMetricFactory &factory = RideMetricFactory::instance();

    // resize and set to zero
    metrics_.fill(0, factory.metricCount());
    count_.fill(0, factory.metricCount());

    // don't open on our account - we should be called with a ride available
    RideFile *f = rideItem_->ride_;
    if (!f) return;


    // ok, lets collect the metrics
    QHash<QString,RideMetricPtr> computed=RideMetric::computeMetrics(rideItem_, Specification(this, f->recIntSecs()), factory.allMetrics());
    // take a deep copy, quick before the thread exits.
    //XXXcomputed.detach();

    // snaffle away all the computed values into the array
    QHashIterator<QString, RideMetricPtr> i(computed);
    while (i.hasNext()) {
        i.next();
        metrics_[i.value()->index()] = i.value()->value();
        count_[i.value()->index()] = i.value()->count();
        double stdmean = i.value()->stdmean();
        double stdvariance = i.value()->stdvariance();
        if (stdmean || stdvariance) {
            stdmean_.insert(i.value()->index(), stdmean);
            stdvariance_.insert(i.value()->index(), stdvariance);
        }
    }

    // clean any bad values
    for(int j=0; j<factory.metricCount(); j++)
        if (std::isinf(metrics_[j]) || std::isnan(metrics_[j])) {
            metrics_[j] = 0.00f;
            count_[j] = 0.00f;
        }
}


double
IntervalItem::getForSymbol(QString name, bool useMetricUnits)
{
    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {

        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m) {
            if (useMetricUnits) return metrics_[m->index()];
            else {
                // little hack to set/get for conversion
                const_cast<RideMetric*>(m)->setValue(metrics_[m->index()]);
                return m->value(useMetricUnits);
            }
        }
    }
    return 0.0f;
}

QString
IntervalItem::getStringForSymbol(QString name, bool useMetricUnits)
{
    QString returning("-");

    const RideMetricFactory &factory = RideMetricFactory::instance();
    if (metrics_.size() && metrics_.size() == factory.metricCount()) {

        // return the precomputed metric value
        const RideMetric *m = factory.rideMetric(name);
        if (m) {

            double value = metrics_[m->index()];
            if (std::isinf(value) || std::isnan(value)) value=0;
            const_cast<RideMetric*>(m)->setValue(value);
            returning = m->toString(useMetricUnits);
        }
    }
    return returning;
}

/*----------------------------------------------------------------------
 * Edit Interval dialog
 *--------------------------------------------------------------------*/
EditIntervalDialog::EditIntervalDialog(QWidget *parent, IntervalItem &interval) :
    QDialog(parent, Qt::Dialog), interval(interval)
{
    setWindowTitle(tr("Edit Interval"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(5 *dpiXFactor);

    // Grid
    QGridLayout *grid = new QGridLayout;

    nameEdit = new QLineEdit(this);
    nameEdit->setText(interval.name);

    if (interval.type == RideFileInterval::USER) {
        fromEdit = new QTimeEdit(this);
        fromEdit->setDisplayFormat("hh:mm:ss");
        fromEdit->setTime(QTime(0,0,0,0).addSecs(interval.start));

        toEdit = new QTimeEdit(this);
        toEdit->setDisplayFormat("hh:mm:ss");
        toEdit->setTime(QTime(0,0,0,0).addSecs(interval.stop));

        colorEdit = new ColorButton(this, interval.name, interval.color);
        colorEdit->setAutoDefault(false);

        istest = new QCheckBox(tr("Performance Test"), this);
        istest->setChecked(interval.istest());
    }


    grid->addWidget(new QLabel("Name"), 0,0);
    grid->addWidget(nameEdit, 0,1);

    if (interval.type == RideFileInterval::USER) {
        grid->addWidget(new QLabel("From"), 1,0);
        grid->addWidget(fromEdit, 1,1);
        grid->addWidget(new QLabel("To"), 2,0);
        grid->addWidget(toEdit, 2,1);
        grid->addWidget(new QLabel("Color"), 3,0);
        grid->addWidget(colorEdit, 3,1);
        grid->addWidget(istest, 4,0);
    }

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    applyButton->setDefault(true);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
EditIntervalDialog::applyClicked()
{
    // set the values to the interval but need to recalc the KM
    // and use the rideItem functions to do this to ensure they
    // get applied to the ridefile interval too (so it gets saved)
    if (interval.rideInterval != NULL) {

        QString iname = nameEdit->text();
        double istart = QTime(0,0,0).secsTo(fromEdit->time());
        double istop = QTime(0,0,0).secsTo(toEdit->time());
        double istartKM = 0;
        double istopKM = 0;

        if (interval.rideItem_ && interval.rideItem_->ride()) {
            istartKM = interval.rideItem_->ride()->timeToDistance(istart);
            istopKM = interval.rideItem_->ride()->timeToDistance(istop);
        }
        QColor icolor = colorEdit->getColor();
        bool itest = istest->isChecked();

        interval.setValues(iname, istart, istop, istartKM, istopKM, icolor, itest);
    }
    accept();
}

void
EditIntervalDialog::cancelClicked()
{
    reject();
}

/*----------------------------------------------------------------------
 * Interval rename dialog
 *--------------------------------------------------------------------*/
RenameIntervalDialog::RenameIntervalDialog(QString &string, QWidget *parent) :
    QDialog(parent, Qt::Dialog), string(string)
{
    setWindowTitle(tr("Rename Intervals"));
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Grid
    QGridLayout *grid = new QGridLayout;
    QLabel *name = new QLabel("Name");

    nameEdit = new QLineEdit(this);
    nameEdit->setText(string);

    grid->addWidget(name, 0,0);
    grid->addWidget(nameEdit, 0,1);

    mainLayout->addLayout(grid);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    applyButton = new QPushButton(tr("&OK"), this);
    cancelButton = new QPushButton(tr("&Cancel"), this);
    buttonLayout->addWidget(cancelButton);
    buttonLayout->addWidget(applyButton);
    mainLayout->addLayout(buttonLayout);

    // connect up slots
    connect(applyButton, SIGNAL(clicked()), this, SLOT(applyClicked()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
RenameIntervalDialog::applyClicked()
{
    // get the values back
    string = nameEdit->text();
    accept();
}

void
RenameIntervalDialog::cancelClicked()
{
    reject();
}
