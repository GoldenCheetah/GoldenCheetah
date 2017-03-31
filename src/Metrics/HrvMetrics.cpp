/*
 *
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

#include "RideMetric.h"
#include "Context.h"
#include "RideItem.h"
#include "RideFile.h"

#define ABS(x) ((x) >= 0 ? (x) : -(x))

class RRNormalFraction : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(RRNormalFraction)

    // NN/RR is the fraction of total RR intervals that are classified
    // as normal-to-normal (NN) intervals and included in the
    // calculation of HRV statistics
public:

    RRNormalFraction()
    {
        setSymbol("nn_rr_fraction");
        setInternalName("NN/RR fraction");
    }

    void initialize()
    {
        setName(tr("Fraction of normal RR intervals"));
        setMetricUnits(tr("pct"));
        setImperialUnits(tr("pct"));
        setType(RideMetric::Average);
        setDescription(tr("Measure of RR readability"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        double total, count;

        bool this_state;
        bool last_state = false;

        XDataSeries *series = item->ride()->xdata("HRV");

        if (series) {

            total = count = 0;

            foreach(XDataPoint *p, series->datapoints)
                {
                    this_state = p->number[1] > 0;
                    if (this_state && last_state)
                        {
                            total++;
                        }
                    last_state = this_state;
                    count++;
                }

            setValue(count > 0 ? total/count*100.0: 100.0);
            setCount(count);
        }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new RRNormalFraction(*this); }
};

static bool nnrrFractionAdded =
    RideMetricFactory::instance().addMetric(RRNormalFraction());


class avnn : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(avnn)

public:

    avnn()
    {
        setSymbol("AVNN");
        setInternalName("AVNN_HRV");
    }

    void initialize()
    {
        setName(tr("Average of all NN intervals"));
        setMetricUnits(tr("msec"));
        setImperialUnits(tr("msec"));
        setType(RideMetric::Average);
        setDescription(tr("Average of all NN intervals"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        double total, count;
        bool last_state = false;
        bool this_state;

        XDataSeries *series = item->ride()->xdata("HRV");

        if (series) {

            total = count = 0;

            foreach(XDataPoint *p, series->datapoints)
                {
                    this_state = p->number[1]>0;
                    if (this_state && last_state)
                        {
                            total += p->number[0];
                            count++;
                        }
                    last_state = this_state;
                }
            setValue(count > 0 ? total/count: total);
            setCount(count);
        }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    bool isRelevantForRide(RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new avnn(*this); }
};

static bool avnnAdded =
    RideMetricFactory::instance().addMetric(avnn());


class sdnn : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(sdnn)

private:

    double stdmean_;

public:

    sdnn()
    {
        setSymbol("SDNN");
        setInternalName("SDNN_HRV");
        stdmean_ = 0.0f;
    }

    void initialize()
    {
        setName(tr("Standard deviation of NN"));
        setMetricUnits(tr("msec"));
        setImperialUnits(tr("msec"));
        setType(RideMetric::StdDev);
        setDescription(tr("Standard deviation of all NN intervals"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        double sum, sum2, count;
        bool last_state = false;
        bool this_state;

        XDataSeries *series = item->ride()->xdata("HRV");

        if (series) {

            sum = sum2 = count = 0;

            foreach(XDataPoint *p, series->datapoints)
                {
                    this_state = p->number[1] > 0;
                    if (this_state && last_state)
                        {
                            sum += p->number[0];
                            sum2 += pow(p->number[0], 2);
                            count++;
                        }
                    last_state = this_state;
                }

            if (count>1)
                {
                    stdmean_ = sum/count;
                    setValue(count > 1 ? sqrt((sum2 - sum*stdmean_)/(count - 1)): 0.0f);
                }
            else
                {
                    setValue(0.0f);
                }
            setCount(count);
        }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    double stdmean(){ return stdmean_; }

    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new sdnn(*this); }
};

static bool sdnnAdded =
    RideMetricFactory::instance().addMetric(sdnn());


class sdann : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(sdann)

private:

    double stdmean_;

public:

    sdann()
    {
        setSymbol("SDANN");
        setInternalName("SDANN_HRV");
        stdmean_ = 0.0f;
    }

    void initialize()
    {
        setName(tr("SDANN"));
        setMetricUnits(tr("msec"));
        setImperialUnits(tr("msec"));
        setType(RideMetric::StdDev);
        setDescription(tr("Standard deviation of all NN intervals in all 5-minute segments of a 24-hour recording"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        double sum, sum2, total, count, n;
        bool last_state = false;
        bool this_state;

        XDataSeries *series = item->ride()->xdata("HRV");
        double tlim = 300.0;

        if (series) {

            sum = sum2 = total = count = n = 0;

            foreach(XDataPoint *p, series->datapoints)
                {
                    if (p->secs >= tlim)
                        {
                            tlim += 300.0;
                            if (n>0)
                                {
                                    total /= n;
                                    sum += total;
                                    sum2 += pow(total, 2);
                                    count++;
                                    total = 0.0;
                                    n = 0;
                                }
                        }

                    this_state = p->number[1] > 0;
                    if (this_state && last_state)
                        {
                            total += p->number[0];
                            n++;
                        }
                    last_state = this_state;
                }

            if (n>0)
                {
                    total /= n;
                    sum += total;
                    sum2 += pow(total, 2);
                    count++;
                }
            if (count>1)
                {
                    stdmean_ = sum/count;
                    setValue(count > 1 ? sqrt((sum2 - sum*stdmean_)/(count - 1)): 0.0f);
                }
            else
                {
                    setValue(0.0f);
                }
            setCount(count);
        }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    double stdmean(){ return stdmean_; }
    
    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new sdann(*this); }
};

static bool sdannAdded =
    RideMetricFactory::instance().addMetric(sdann());


class sdnnidx : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(sdnnidx)

public:

    sdnnidx()
    {
        setSymbol("SDNNIDX");
        setInternalName("SDNNIDX_HRV");
    }

    void initialize()
    {
        setName(tr("SDNNIDX"));
        setMetricUnits(tr("msec"));
        setImperialUnits(tr("msec"));
        setType(RideMetric::Average);
        setDescription(tr("Average of the standard deviations of NN intervals in all 5-minute segments of a 24-hour recording"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {

        double sum, sum2, total, count, n;
        bool last_state = false;
        bool this_state;
        double tlim = 300.0;

        XDataSeries *series = item->ride()->xdata("HRV");

        if (series) {

            sum = sum2 = total = count = n = 0;

            foreach(XDataPoint *p, series->datapoints)
                {
                    if (p->secs >= tlim)
                        {
                            tlim += 300.0;
                            if (n>0)
                                {
                                    total += sqrt((sum2 - sum*sum/n)/(n-1));
                                    count++;
                                    // Reset
                                    sum = sum2 = 0.0;
                                    n = 0;
                                }
                        }

                    this_state = p->number[1]>0;
                    if (this_state && last_state)
                        {
                            sum += p->number[0];
                            sum2 += pow(p->number[0], 2);
                            n++;
                        }
                    last_state = this_state;
                }
            if (n>0)
                {
                    total += sqrt((sum2 - sum*sum/n)/(n-1));
                    count++;
                }
            setValue(count > 0 ? total/count: total);
            setCount(count);
        }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new sdnnidx(*this); }
};

static bool sdnnidxAdded =
    RideMetricFactory::instance().addMetric(sdnnidx());


class rmssd : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(rmssd)

public:

    rmssd()
    {
        setSymbol("rMSSD");
        setInternalName("rMSSD_HRV");
    }

    void initialize()
    {
        setName(tr("rMSSD"));
        setMetricUnits(tr("msec"));
        setImperialUnits(tr("msec"));
        setType(RideMetric::MeanSquareRoot);
        setDescription(tr("Square root of the mean of the squares of differences between adjacent NN intervals"));
    }

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        double sum, count;

        XDataSeries *series = item->ride()->xdata("HRV");

        if (series && series->datapoints.count() > 2 )
            {
                sum = count = 0;

                for (int i=2; i < series->datapoints.count(); i++)
                    if (
                        series->datapoints[i]->number[1] > 0 &&
                        series->datapoints[i-1]->number[1] > 0 &&
                        series->datapoints[i-2]->number[1] > 0
                        )
                        {
                            sum += pow(series->datapoints[i]->number[0] - series->datapoints[i-1]->number[0], 2);
                            count++;
                        }
                setValue(count > 1 ? sqrt(sum/count): 0);
                setCount(count);
            }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    bool isRelevantForRide(const RideItem *) const { return true; }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new rmssd(*this); }
};

static bool rmssdAdded =
    RideMetricFactory::instance().addMetric(rmssd());


class pnnx : public RideMetric {
    Q_DECLARE_TR_FUNCTIONS(pnnx)

public:
    double msec;

    pnnx(double msec_val)
    {
        msec = msec_val;
        setSymbol(QString("pNN").append(QString::number(msec, 'f', 0)));
        setInternalName(QString("pNN_HRV").insert(3, QString::number(msec, 'f', 0)));
    };

    void compute(RideItem *item, Specification, const QHash<QString,RideMetric*> &) {
        int nnx, count;
        XDataSeries *series = item->ride()->xdata("HRV");

        if (series && series->datapoints.count() > 2 )
            {
                nnx = count = 0;

                for (int i=2; i < series->datapoints.count(); i++)
                    {
                        if (
                            series->datapoints[i]->number[1] > 0 &&
                            series->datapoints[i-1]->number[1] > 0 &&
                            series->datapoints[i-2]->number[1] > 0
                            )
                            {
                                if (ABS(series->datapoints[i]->number[0] - series->datapoints[i-1]->number[0]) > msec)
                                    {
                                        nnx++;
                                    }
                                count++;
                            }
                    }

                setValue(count > 0 ? nnx/count*100.0 : 0.0);
                setCount(count);
            }
        else {
            setValue(RideFile::NIL);
            setCount(0);
        }
    }

    void initialize()
    {
        setName(QString(tr("pNN")).append(QString::number(msec, 'f', 0)));
        setMetricUnits(tr("pct"));
        setImperialUnits(tr("pct"));
        setType(RideMetric::Average);
        setDescription(QString(tr("Percentage of differences between adjacent NN intervals that are greater than %1 ms").arg(QString::number(msec, 'f', 0))));
    }

    MetricClass classification() const { return Undefined; }
    MetricValidity validity() const { return Unknown; }
    RideMetric *clone() const { return new pnnx(*this); }
    bool isRelevantForRide(const RideItem *) const { return true; }
};

static bool addPnnx()
{
    for (int i=1; i<10; i++)
        { RideMetricFactory::instance().addMetric(pnnx(i*5));}
    return true;
}

static bool pnnxadded = addPnnx();
