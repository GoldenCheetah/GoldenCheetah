/*
 * Copyright (c) 2010 Greg Lonnon (greg.lonnon@gmail.com)
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

#include "WorkoutWizard.h"

#include "qwt_plot.h"
#include "qwt_plot_curve.h"
#include <limits>


// hack...  Need to get the CP and zones for metrics
MainWindow *hackMW;

/// workout plot
class WorkoutPlot: public QwtPlot
{
    QwtPlotCurve *workoutCurve;
public:
    WorkoutPlot()
    {
        workoutCurve = new QwtPlotCurve();
        setTitle("Workout Chart");
        QPen pen = QPen(Qt::blue,1.0);
        workoutCurve->setPen(pen);
        QColor brush_color = QColor(124, 91, 31);
        brush_color.setAlpha(64);
        workoutCurve->setBrush(brush_color);
        workoutCurve->attach(this);
    }
    void setYAxisTitle(QString title)
    {
        setAxisTitle(QwtPlot::yLeft, title);
    }
    void setXAxisTitle(QString title)
    {
        setAxisTitle(QwtPlot::xBottom,title);
    }
    void setData(QwtData &data)
    {
        workoutCurve->setData(data);
    }
};

//// Workout Editor

WorkoutEditorBase::WorkoutEditorBase(QStringList &colms, QWidget *parent =0 ) :QFrame(parent)
{
    QVBoxLayout *layout = new QVBoxLayout();
    QHBoxLayout *row1Layout = new QHBoxLayout();
    table = new QTableWidget();

    table->setColumnCount(colms.count());
    table->setHorizontalHeaderLabels(colms);
    table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    table->setShowGrid(true);
    table->setAlternatingRowColors(true);
    table->resizeColumnsToContents();
    row1Layout->addWidget(table);

    connect(table,SIGNAL(cellChanged(int,int)),this,SLOT(cellChanged(int,int)));


    QHBoxLayout *row2Layout = new QHBoxLayout();
    QPushButton *delButton = new QPushButton();
    delButton->setText(tr("Delete"));
    delButton->setToolTip(tr("Delete the highlighted row"));
    connect(delButton,SIGNAL(clicked()),this,SLOT(delButtonClicked()));
    row2Layout->addWidget(delButton);
    QPushButton *addButton = new QPushButton();
    addButton->setText(tr("Add"));
    addButton->setToolTip(tr("Add row at end"));
    connect(addButton,SIGNAL(clicked()),this,SLOT(addButtonClicked()));
    row2Layout->addWidget(addButton);
    QPushButton *insertButton = new QPushButton();
    insertButton->setText(tr("Insert"));
    insertButton->setToolTip(tr("Add a Lap below the highlighted row"));
    connect(insertButton,SIGNAL(clicked()),this,SLOT(insertButtonClicked()));
    row2Layout->addWidget(insertButton);
    QPushButton *lapButton = new QPushButton();
    lapButton->setText(tr("Lap"));
    lapButton->setToolTip(tr("Add a Lap below the highlighted row"));
    row2Layout->addWidget(lapButton);
    connect(lapButton,SIGNAL(clicked()),this,SLOT(lapButtonClicked()));
    layout->addLayout(row1Layout);
    layout->addLayout(row2Layout);
    setLayout(layout);
}



void WorkoutEditorAbs::insertDataRow(int row)
{
    table->insertRow(row);
    // minutes colm can be doubles
    table->setItem(row,0,new WorkoutItemDouble());
    // wattage must be integer
    table->setItem(row,1,new WorkoutItemInt());
}

void WorkoutEditorRel::insertDataRow(int row)
{
    table->insertRow(row);
    // minutes colm can be doubles
    table->setItem(row,0,new WorkoutItemDouble());
    // precentage of ftp
    table->setItem(row,1,new WorkoutItemDouble());
    // current ftp
    table->setItem(row,1,new WorkoutItemInt());
}

template<int minGrade, int maxGrade>
class WorkoutItemDoubleRange : public WorkoutItemDouble
{
    int min, max;
public:
    WorkoutItemDoubleRange() : min(minGrade), max(maxGrade) {}
    QString validateData(const QString &text)
    {
        double d = text.toDouble();
        d = d > max ? max : d;
        d = d < min ? min : d;
        return QString::number(d);
    }

};

void WorkoutEditorGradient::insertDataRow(int row)
{
    table->insertRow(row);
    // distance
    table->setItem(row,0,new WorkoutItemDouble());
    // grade
    table->setItem(row,1,new WorkoutItemDoubleRange<-5,5>());
}

///  Workout Summary

void WorkoutMetricsSummary::updateMetrics(QStringList &order, QHash<QString,RideMetricPtr>  &metrics)
{
    int row = 0;

    foreach(QString name, order)
    {
        RideMetricPtr rmp = metrics[name];
        if(!metricMap.contains(name))
        {
            QLabel *label = new QLabel((rmp->name()) + ":");
            QLabel *lcd = new QLabel();
            metricMap[name] = QPair<QLabel*,QLabel*>(label,lcd);
            layout->addWidget(label,metricMap.size(),0);
            layout->addWidget(lcd,metricMap.size(),1);
        }
        QLabel *lcd = metricMap[name].second;
        if(name == "time_riding")
        {
            QTime time;

            time = time.addSecs(rmp->value(true));
            QString s = time.toString("HH:mm:ss");
            //qDebug() << s << " " << time.second();
            lcd->setText(s);
        }
        else
        {
            lcd->setText(QString::number(rmp->value(true),'f',rmp->precision()) + " " + (rmp->units(true)) );
        }
        //qDebug() << name << ":" << (int)rmp->value(true);
        row++;
    }
}

void WorkoutMetricsSummary::updateMetrics(QMap<QString, QString> &map)
{
    QMap<QString, QString>::iterator i = map.begin();
    int row = 0;
    while(i != map.end())
    {
        if(!metricMap.contains(i.key()))
        {
            QLabel *label = new QLabel((i.key() + ":"));
            QLabel *value = new QLabel();
            metricMap[i.key()] = QPair<QLabel*,QLabel*>(label,value);
            layout->addWidget(label,metricMap.size(),0);
            layout->addWidget(value,metricMap.size(),1);
        }
        QLabel *value = metricMap[i.key()].second;
        value->setText(i.value());
        row++;
        ++i;
    }
}

/// WorkoutTypePage

WorkoutTypePage::WorkoutTypePage(QWidget *parent =0) : QWizardPage(parent)
{
}

void WorkoutTypePage::initializePage()
{
    setTitle(tr("Workout Creator"));
    setSubTitle(tr("Select the workout type to be created"));
    buttonGroupBox = new QButtonGroup(this);
    absWattageRadioButton = new QRadioButton(tr("Absolute Wattage"));
    absWattageRadioButton->click();
    relWattageRadioButton = new QRadioButton(tr("% FTP Wattage"));
    gradientRadioButton = new QRadioButton(tr("Gradient"));

    if (hackMW->rideItem()) {
        QString s = hackMW->rideItem()->ride()->startTime().toLocalTime().toString();
        QString importStr = "Import Selected Ride (" + s + ")";
        importRadioButton = new QRadioButton((importStr));
    } else {
        importRadioButton = new QRadioButton("No ride selected");
    }
    QVBoxLayout *groupBoxLayout = new QVBoxLayout();

    groupBoxLayout->addWidget(absWattageRadioButton);
    groupBoxLayout->addWidget(relWattageRadioButton);
    groupBoxLayout->addWidget(gradientRadioButton);
    groupBoxLayout->addWidget(importRadioButton);
    registerField("absWattage",absWattageRadioButton);
    registerField("relWattage",relWattageRadioButton);
    registerField("gradientWattage",gradientRadioButton);
    registerField("import",importRadioButton);
    setLayout(groupBoxLayout);
}

int WorkoutTypePage::nextId() const
{
    if(absWattageRadioButton->isChecked())
        return WorkoutWizard::WW_AbsWattagePage;
    else if (relWattageRadioButton->isChecked())
        return WorkoutWizard::WW_RelWattagePage;
    else if (gradientRadioButton->isChecked())
        return WorkoutWizard::WW_GradientPage;
    else if (importRadioButton->isChecked())
        return WorkoutWizard::WW_ImportPage;
    return WorkoutWizard::WW_AbsWattagePage;
}


//// AbsWattagePage

AbsWattagePage::AbsWattagePage(QWidget *parent = NULL) : WorkoutPage(parent)
{}

void AbsWattagePage::initializePage()
{
    setTitle("Workout Wizard");
    setSubTitle("Absolute Wattage Workout Creator");
    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);
    QStringList colms;
    colms.append(tr("Minutes"));
    colms.append(tr("Wattage"));
    we = new WorkoutEditorAbs(colms);
    layout->addWidget(we);
    QVBoxLayout *summaryLayout = new QVBoxLayout();
    metricsSummary = new WorkoutMetricsSummary();
    summaryLayout->addWidget(metricsSummary);
    plot = new WorkoutPlot();
    plot->setYAxisTitle("Wattage");
    plot->setXAxisTitle("Time (minutes)");
    plot->setAxisScale(QwtPlot::yLeft,0,500,0);
    plot->setAxisScale(QwtPlot::xBottom,0,120,0);
    summaryLayout->addWidget(plot);
    summaryLayout->addStretch(1);
    layout->addLayout(summaryLayout);
    layout->addStretch(1);
    connect(we,SIGNAL(dataChanged()),this,SLOT(updateMetrics()));
    updateMetrics();
}

void AbsWattagePage::updateMetrics()
{
    QVector<QPair<QString,QString> > data;
    QwtArray<double> x;
    QwtArray<double> y;

    we->rawData(data);

    int curSecs = 0;
    // create rideFile
    boost::shared_ptr<RideFile> workout(new RideFile());
    workout->setRecIntSecs(1);
    double curMin = 0;
    for(int i = 0; i < data.size() ; i++)
    {
        if(data[i].first == "LAP") continue;

        double min = data[i].first.toDouble();
        double watts = data[i].second.toDouble();
        int secs = min * 60;
        for(int j = 0; j < secs; j++)
        {
            RideFilePoint rfp;
            rfp.secs = curSecs++;
            rfp.watts = watts;
            rfp.cad = 90;
            workout->appendPoint(rfp);
        }

        x.append(curMin);
        y.append(watts);
        curMin += min;
        x.append(curMin);
        y.append(watts);

    }
    // replot workoutplot
    QwtArrayData d(x,y);
    plot->setAxisAutoScale(QwtPlot::yLeft);
    plot->setAxisAutoScale(QwtPlot::xBottom);
    plot->setData(d);
    plot->replot();

    // calculate bike score, xpower
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *rm = factory.rideMetric("skiba_xpower");
    QStringList metrics;
    metrics.append("time_riding");
    metrics.append("total_work");
    metrics.append("average_power");
    metrics.append("skiba_bike_score");
    metrics.append("skiba_xpower");
    QHash<QString,RideMetricPtr> results = rm->computeMetrics(NULL,&*workout,hackMW->zones(),hackMW->hrZones(),metrics);
    metricsSummary->updateMetrics(metrics,results);
}

void AbsWattagePage::SaveWorkout()
{
    QString workoutDir = appsettings->value(this,GC_WORKOUTDIR).toString();

    QString filename = QFileDialog::getSaveFileName(this,QString("Save Workout"),
                                                    workoutDir,"Computrainer Format *.erg");
    if(!filename.endsWith(".erg"))
    {
        filename.append(".erg");
    }
    // open the file
    QFile f(filename);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);
    // create the header
    SaveWorkoutHeader(stream,f.fileName(),QString("golden cheetah"),QString("MINUTES WATTS"));
    QVector<QPair<QString, QString> > rawData;
    we->rawData(rawData);
    double currentX = 0;
    stream << "[COURSE DATA]" << endl;
    QPair<QString, QString > p;
    foreach (p,rawData)
    {
        if(p.first == "LAP")
        {
            stream << currentX << " LAP" << endl;
        }
        else
        {
            stream << currentX << " " << p.second << endl;
            currentX += p.first.toDouble();
            stream << currentX << " " << p.second << endl;
        }
    }
    stream << "[END COURSE DATA]" << endl;
}

/// RelativeWattagePage

RelWattagePage::RelWattagePage(QWidget *parent = NULL) : WorkoutPage(parent)
{}

void RelWattagePage::initializePage()
{
    int zoneRange = hackMW->zones()->whichRange(QDate::currentDate());
    ftp = hackMW->zones()->getCP(zoneRange);

    setTitle("Workout Wizard");
    QString subTitle = "Relative Wattage Workout Wizard, current CP60 = " + QString::number(ftp);
    setSubTitle(subTitle);

    plot = new WorkoutPlot();
    plot->setYAxisTitle(tr("% of FTP"));
    plot->setXAxisTitle(tr("Time (minutes)"));
    plot->setAxisScale(QwtPlot::yLeft,0,200,0);
    plot->setAxisScale(QwtPlot::xBottom,0,120,0);

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);
    QStringList colms;
    colms.append(tr("Minutes"));
    colms.append(tr("% of FTP"));
    colms.append(tr("Wattage"));
    we = new WorkoutEditorRel(colms,ftp);
    layout->addWidget(we);
    QVBoxLayout *summaryLayout = new QVBoxLayout();
    metricsSummary = new WorkoutMetricsSummary();
    summaryLayout->addWidget(metricsSummary);
    summaryLayout->addWidget(plot,1);
    layout->addLayout(summaryLayout);
    layout->addStretch(1);
    connect(we,SIGNAL(dataChanged()),this,SLOT(updateMetrics()));
    updateMetrics();
}

void RelWattagePage::updateMetrics()
{
    QVector<QPair<QString,QString> > data;
    QwtArray<double> x;
    QwtArray<double> y;

    we->rawData(data);

    int curSecs = 0;
    // create rideFile
    boost::shared_ptr<RideFile> workout(new RideFile());
    workout->setRecIntSecs(1);
    for(int i = 0; i < data.size() ; i++)
    {
        if(data[i].first == "LAP") continue;
        double min = data[i].first.toDouble();
        double percentFtp = data[i].second.toDouble();
        int secs = min * 60;
        x.append(curSecs/60);
        y.append(percentFtp);
        for(int j = 0; j < secs; j++)
        {
            RideFilePoint rfp;
            rfp.secs = curSecs++;
            rfp.watts = percentFtp * ftp /100;
            rfp.cad = 90;
            workout->appendPoint(rfp);
        }
        x.append(curSecs/60);
        y.append(percentFtp);
    }

    // replot workoutplot
    QwtArrayData d(x,y);
    plot->setAxisAutoScale(QwtPlot::yLeft);
    plot->setAxisAutoScale(QwtPlot::xBottom);
    plot->setData(d);
    plot->replot();

    // calculate bike score, xpower
    const RideMetricFactory &factory = RideMetricFactory::instance();
    const RideMetric *rm = factory.rideMetric("skiba_xpower");

    QStringList metrics;
    metrics.append("time_riding");
    metrics.append("total_work");
    metrics.append("average_power");
    metrics.append("skiba_bike_score");
    metrics.append("skiba_xpower");
    QHash<QString,RideMetricPtr> results = rm->computeMetrics(NULL,&*workout,hackMW->zones(),hackMW->hrZones(),metrics);
    metricsSummary->updateMetrics(metrics,results);
}

void RelWattagePage::SaveWorkout()
{
    QString workoutDir = appsettings->value(this,GC_WORKOUTDIR).toString();

    QString filename = QFileDialog::getSaveFileName(this,QString("Save Workout"),
                                                    workoutDir,"Computrainer Format *.mrc");
    if(!filename.endsWith(".mrc"))
    {
        filename.append(".mrc");
    }
    // open the file
    QFile f(filename);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);
    // create the header
    SaveWorkoutHeader(stream,f.fileName(),QString("golden cheetah"),QString("MINUTES PERCENTAGE"));
    QVector<QPair<QString, QString> > rawData;
    we->rawData(rawData);
    double currentX = 0;
    stream << "[COURSE DATA]" << endl;
    QPair<QString, QString > p;
    foreach (p,rawData)
    {
        if(p.first == "LAP")
        {
            stream << currentX << " LAP" << endl;
        }
        else
        {
            stream << currentX << " " << p.second << endl;
            currentX += p.first.toDouble();
            stream << currentX << " " << p.second << endl;
        }
    }
    stream << "[END COURSE DATA]" << endl;
}

/// GradientPage
GradientPage::GradientPage(QWidget *parent = NULL) : WorkoutPage(parent)
{}

void GradientPage::initializePage()
{
    int zoneRange = hackMW->zones()->whichRange(QDate::currentDate());
    ftp = hackMW->zones()->getCP(zoneRange);
    metricUnits = (appsettings->value(NULL, GC_UNIT).toString() == "Metric");
    setTitle("Workout Wizard");

    setSubTitle("Manually crate a workout based on gradient (slope) and distance, maxium grade is 5.");

    QHBoxLayout *layout = new QHBoxLayout();
    setLayout(layout);
    QStringList colms;

    colms.append(tr(metricUnits ? "KM" : "Miles"));
    colms.append(tr("Grade"));
    we = new WorkoutEditorGradient(colms);
    layout->addWidget(we);
    QVBoxLayout *summaryLayout = new QVBoxLayout();
    metricsSummary = new WorkoutMetricsSummary();
    summaryLayout->addWidget(metricsSummary);
    summaryLayout->addStretch(1);
    layout->addLayout(summaryLayout);
    layout->addStretch(1);
    connect(we,SIGNAL(dataChanged()),this,SLOT(updateMetrics()));
    updateMetrics();
}

void GradientPage::updateMetrics()
{
    QVector<QPair<QString,QString> > data;
    we->rawData(data);

    int totalDistance = 0;
    double gain = 0;
    int elevation = 0;
    // create rideFile
    boost::shared_ptr<RideFile> workout(new RideFile());
    workout->setRecIntSecs(1);
    for(int i = 0; i < data.size() ; i++)
    {
        if(data[i].first == "LAP") continue;
        double distance = data[i].first.toDouble();
        double grade = data[i].second.toDouble();
        double delta = distance  * (metricUnits ? 1000 : 5120) * grade /100;
        gain += (delta > 0) ? delta : 0;
        elevation += delta;
        totalDistance += distance;
    }
    QMap<QString,QString> metricSummaryMap;
    QString s = (metricUnits ? "KM" : "Miles");
    metricSummaryMap[s] = QString::number(totalDistance);
    s = (metricUnits ? "Meters Gained" : "Feet Gained");
    metricSummaryMap[s] = QString::number(gain);
    metricsSummary->updateMetrics(metricSummaryMap);
}

void GradientPage::SaveWorkout()
{
    QString workoutDir = appsettings->value(this,GC_WORKOUTDIR).toString();

    QString filename = QFileDialog::getSaveFileName(this,QString("Save Workout"),
                                                    workoutDir,"Computrainer Format *.crs");
    if(!filename.endsWith(".crs"))
    {
        filename.append(".crs");
    }
    // open the file
    QFile f(filename);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);
    // create the header
    SaveWorkoutHeader(stream,f.fileName(),QString("golden cheetah"),QString("DISTANCE GRADE WIND"));
    QVector<QPair<QString, QString> > rawData;
    we->rawData(rawData);
    double currentX = 0;
    stream << "[COURSE DATA]" << endl;
    QPair<QString, QString > p;
    foreach (p,rawData)
    {
        currentX += p.first.toDouble();
        stream << currentX << " " << p.second << " 0" << endl;
    }
    stream << "[END COURSE DATA]" << endl;
}



ImportPage::ImportPage(QWidget *parent = 0) : WorkoutPage(parent)
{}

void ImportPage::initializePage()
{

        setTitle("Workout Wizard");
        setSubTitle("Import current ride as a Gradient Ride (slope based)");
        setFinalPage(true);
        QVBoxLayout *layout = new QVBoxLayout();
        plot = new WorkoutPlot();
        metricUnits = (appsettings->value(NULL, GC_UNIT).toString() == "Metric");
        QString s = (metricUnits ? "KM" : "Miles");
        QString distance = QString("Distance (") + s + QString(")");
        plot->setXAxisTitle(distance);
        s = (metricUnits ? "Meters" : "Feet");
        QString elevation = QString("elevation (") + s + QString(")");
        plot->setYAxisTitle(elevation);
        RideItem *rideItem = hackMW->rideItem();

        if(rideItem == NULL)
            return;

        foreach(RideFilePoint *rfp,rideItem->ride()->dataPoints())
        {
            rideData.append(QPair<double,double>(rfp->km,rfp->alt));
        }
        layout->addWidget(plot,1);

        QGroupBox *spinGroupBox = new QGroupBox();
        spinGroupBox->setTitle(tr("Ride Smoothing Parameters"));
        QLabel *gradeLabel = new QLabel("Maximum Grade");
        gradeBox = new QSpinBox();
        gradeBox->setValue(5);
        gradeBox->setMaximum(8);
        gradeBox->setMinimum(0);
        gradeBox->setToolTip(tr("Maximum supported grade is 8"));
        connect(gradeBox,SIGNAL(valueChanged(int)),this,SLOT(updatePlot()));

        QLabel *segmentLabel = new QLabel("Segment Length");
        segmentBox = new QSpinBox();
        segmentBox->setMinimum(0);
        segmentBox->setMaximum((metricUnits ? 1000 : 5120));
        segmentBox->setValue((metricUnits ? 1000 : 5120)/2);
        segmentBox->setSingleStep((metricUnits ? 1000 : 5120)/100);
        s = QString("Segment length is based on ") + QString(metricUnits ? "meters": "feet");
        segmentBox->setToolTip((s));
        connect(segmentBox,SIGNAL(valueChanged(int)),this,SLOT(updatePlot()));
        QHBoxLayout *bottomLayout = new QHBoxLayout();
        QGridLayout *spinBoxLayout = new QGridLayout();
        spinBoxLayout->addWidget(gradeLabel,0,0);
        spinBoxLayout->addWidget(gradeBox,0,1);
        spinBoxLayout->addWidget(segmentLabel,1,0);
        spinBoxLayout->addWidget(segmentBox,1,1);
        spinGroupBox->setLayout(spinBoxLayout);
        bottomLayout->addWidget(spinGroupBox);
        metricsSummary = new WorkoutMetricsSummary();
        bottomLayout->addWidget(metricsSummary);

        layout->addLayout(bottomLayout);
        setLayout(layout);
        updatePlot();
}

void ImportPage::updatePlot()
{
    QwtArray<double> x;
    QwtArray<double> y;
    QPair<double,double> p;

    int segmentLength = segmentBox->value();
    double maxSlope = gradeBox->value();

    double curAlt;
    rideProfile.clear();
    double startDistance = 0;
    double curDistance = 0;
    double startAlt = rideData.at(0).second * (!metricUnits ? FEET_PER_METER : 1);
    double totalDistance = 0;
    foreach(p, rideData)
    {
        totalDistance = p.first * (!metricUnits ? MILES_PER_KM : 1);
        curAlt = p.second * (!metricUnits ? FEET_PER_METER : 1);
        curDistance = (totalDistance - startDistance) * (metricUnits ? 1000 : 5120);
        if(curDistance > segmentLength)
        {
            double slope = (curAlt - startAlt) / curDistance * 100;
            slope = std::min(slope, maxSlope);
            double alt = startAlt + (slope * curDistance / 100);
            x.append(totalDistance);
            y.append(alt);
            rideProfile.append(QPair<double,double>(totalDistance,slope));
            startDistance = totalDistance;
            startAlt = curAlt;
        }
    }

    double gain= 0;
    double alt;
    double prevAlt = 0;
    foreach(alt, y)
    {
        gain += (alt > prevAlt) &&(prevAlt != 0) ? (alt - prevAlt) : 0;
        prevAlt = alt;
    }
    QMap<QString, QString> metrics;
    metrics["Elevation Climbed"] = QString::number((int)gain);
    metrics["Distance"] = QString::number(totalDistance,'f',1);
    metricsSummary->updateMetrics(metrics);

    QwtArrayData profileData(x,y);
    plot->setData(profileData);
    plot->replot();

    update();
}

void ImportPage::SaveWorkout()
{
    QString workoutDir = appsettings->value(this,GC_WORKOUTDIR).toString();
    QString filename = QFileDialog::getSaveFileName(this,QString("Save Workout"),
                                                    workoutDir,"Computrainer Format *.crs");
    if(!filename.endsWith(".crs"))
    {
        filename.append(".crs");
    }
    // open the file
    QFile f(filename);
    f.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream stream(&f);
    // create the header
    SaveWorkoutHeader(stream,f.fileName(),QString("golden cheetah"),QString("DISTANCE GRADE WIND"));
    stream << "[COURSE DATA]" << endl;
    QPair<double,double> p;
    double prevDistance = 0;
    foreach (p,rideProfile)
    {
        stream << p.first - prevDistance << " " << p.second <<" 0" << endl;
        prevDistance = p.first;
    }
    stream << "[END COURSE DATA]" << endl;
}

WorkoutWizard::WorkoutWizard(QWidget *parent) :QWizard(parent)
{
    hackMW = (MainWindow *)parent;
    setPage(WW_WorkoutTypePage, new WorkoutTypePage());
    setPage(WW_AbsWattagePage, new AbsWattagePage());
    setPage(WW_RelWattagePage, new RelWattagePage());
    setPage(WW_GradientPage, new GradientPage());
    setPage(WW_ImportPage, new ImportPage());
    this->setStartId(WW_WorkoutTypePage);

}
// called at the end of the wizard...
void WorkoutWizard::accept()
{
    WorkoutPage *page = (WorkoutPage *)this->currentPage();
    page->SaveWorkout();
    done(0);
}
