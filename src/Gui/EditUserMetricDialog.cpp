/*
 * Copyright (c) 2015 Mark Liversedge (liversedge@gmail.com)
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

#include "UserMetricSettings.h"

#include "LTMTool.h" // formula edit
#include "Context.h"
#include "Athlete.h"
#include "MainWindow.h"
#include "AthleteTab.h"
#include "RideNavigator.h"
#include "DataFilter.h"
#include "Zones.h"
#include "HrZones.h"
#include "RideMetric.h"
#include "HelpWhatsThis.h"

#include <QFont>
#include <QFontMetrics>
#include <QMessageBox>

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}
// although we edit global user metrics we do so in the current
// context, using the current ride as a basis for the computation
// and refreshing it when the current ride changes etc.
EditUserMetricDialog::EditUserMetricDialog(QWidget *parent, Context *context, UserMetricSettings &here)
    : QDialog(parent, Qt::Dialog), context(context), settings(here)
{
    setWindowTitle(tr("User Defined Metric"));
    setMinimumHeight(680 *dpiYFactor);

    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::Preferences_Metrics_UserMetrics));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Setup all the widgets
    QLabel *symbolLabel = new QLabel(tr("Symbol"), this);
    symbol = new QLineEdit(this);
    symbol->setText(settings.symbol);

    QLabel *nameLabel = new QLabel(tr("Name"), this);
    name = new QLineEdit(this);
    name->setText(settings.name);

    QLabel *unitsMetricLabel = new QLabel(tr("Metric Units"), this);
    unitsMetric = new QLineEdit(this);
    unitsMetric->setText(settings.unitsMetric);

    QLabel *unitsImperialLabel = new QLabel(tr("Imperial Units"), this);
    unitsImperial = new QLineEdit(this);
    unitsImperial->setText(settings.unitsImperial);

    QLabel *descriptionLabel = new QLabel(tr("Description"), this);
    description = new QTextEdit(this);
    description->setText(settings.description);

    QLabel *typeLabel = new QLabel(tr("Type"), this);
    type = new QComboBox(this);
    type->addItem(tr("Total"));
    type->addItem(tr("Average"));
    type->addItem(tr("Peak"));
    type->addItem(tr("Low"));
    type->setCurrentIndex(settings.type);

    QLabel *conversionLabel = new QLabel(tr("Conversion Factor"), this);
    conversion = new QDoubleSpinBox(this);
    conversion->setDecimals(3);
    conversion->setValue(settings.conversion);

    QLabel *conversionSumLabel = new QLabel(tr("Conversion Sum"), this);
    conversionSum = new QDoubleSpinBox(this);
    conversionSum->setDecimals(3);
    conversionSum->setValue(settings.conversionSum);

    QLabel *precisionLabel = new QLabel(tr("Precision"), this);
    precision = new QDoubleSpinBox(this);
    precision->setDecimals(0);
    precision->setValue(settings.precision);

    aggzero = new QCheckBox(tr("Aggregate Zero"));
    aggzero->setChecked(settings.aggzero);

    istime = new QCheckBox(tr("Time"));
    istime->setChecked(settings.istime);

    QLabel *formulaEditLabel = new QLabel(tr("Program"), this);
    formulaEdit = new DataFilterEdit(this, context);
    QFont courier("Courier", QFont().pointSize());
    QFontMetrics fm(courier);
    formulaEdit->setFont(courier);
    formulaEdit->setTabStopDistance(4 * fm.horizontalAdvance(' ')); // 4 char tabstop
    formulaEdit->setText(settings.program);
    // get suitably formated list XXX XXX ffs, refactor this into FormulEdit !!! XXX XXX
    QList<QString> list;
    QString last;
    SpecialFields sp;

    // get sorted list
    QStringList names = context->rideNavigator->logicalHeadings;

    // start with just a list of functions
    list = DataFilter::builtins(context);

    // ridefile data series symbols
    list += RideFile::symbols();

    // add special functions (older code needs fixing !)
    list << "config(cranklength)";
    list << "config(cp)";
    list << "config(aetp)";
    list << "config(ftp)";
    list << "config(w')";
    list << "config(pmax)";
    list << "config(cv)";
    list << "config(aetv)";
    list << "config(sex)";
    list << "config(dob)";
    list << "config(height)";
    list << "config(weight)";
    list << "config(lthr)";
    list << "config(aethr)";
    list << "config(maxhr)";
    list << "config(rhr)";
    list << "config(units)";
    list << "const(e)";
    list << "const(pi)";
    list << "daterange(start)";
    list << "daterange(stop)";
    list << "ctl";
    list << "tsb";
    list << "atl";
    list << "sb(BikeStress)";
    list << "lts(BikeStress)";
    list << "sts(BikeStress)";
    list << "rr(BikeStress)";
    list << "tiz(power, 1)";
    list << "tiz(hr, 1)";
    list << "best(power, 3600)";
    list << "best(hr, 3600)";
    list << "best(cadence, 3600)";
    list << "best(speed, 3600)";
    list << "best(torque, 3600)";
    list << "best(isopower, 3600)";
    list << "best(xpower, 3600)";
    list << "best(vam, 3600)";
    list << "best(wpk, 3600)";

    std::sort(names.begin(), names.end(), insensitiveLessThan);

    foreach(QString name, names) {

        // handle dups
        if (last == name) continue;
        last = name;

        // Handle bikescore tm
        if (name.startsWith("BikeScore")) name = QString("BikeScore");

        //  Always use the "internalNames" in Filter expressions
        name = sp.internalName(name);

        // we do very little to the name, just space to _ and lower case it for now...
        name.replace(' ', '_');
        list << name;
    }

    // set new list
    // create an empty completer, configchanged will fix it
    DataFilterCompleter *completer = new DataFilterCompleter(list, this);
    formulaEdit->setCompleter(completer);
    errors= new QLabel(this);
    errors->setStyleSheet("color: red");
    connect(formulaEdit, SIGNAL(syntaxErrors(QStringList&)), this, SLOT(setErrors(QStringList&)));

    QLabel *eval = new QLabel(tr("Evaluates"), this);
    QLabel *metric = new QLabel(tr("Metric"), this);
    QLabel *imperial = new QLabel(tr("Imperial"), this);
    //QLabel *elapsed = new QLabel(tr("Elapsed"), this);

    mValue = new QLabel("", this);
    iValue = new QLabel("", this);
    elapsed = new QLabel("", this);

    okButton = new QPushButton(tr("OK"));
    cancelButton = new QPushButton(tr("Cancel"));
    test = new QPushButton(tr("Test"));

    QGridLayout *head = new QGridLayout;

    // first row; symbol and name
    head->addWidget(symbolLabel, 0,0);
    head->addWidget(symbol, 0,1);
    head->addWidget(nameLabel, 0,2);
    head->addWidget(name, 0,3,1,2);

    // second row; type and top of description
    head->addWidget(typeLabel, 1,0);
    head->addWidget(type, 1,1);
    head->addWidget(descriptionLabel, 1,2);
    head->addWidget(description, 1,3,2,3);

    // third row; precision and middle of description
    head->addWidget(precisionLabel, 2,0);
    head->addWidget(precision, 2,1);

    // fourth row; aggzero and istime and bottom of description
    head->addWidget(istime, 3,1);
    head->addWidget(aggzero, 3,2);

    // fifth row; labels for units etc
    head->addWidget(unitsMetricLabel, 4,0);
    head->addWidget(unitsImperialLabel, 4,1);
    head->addWidget(conversionLabel, 4,3);
    head->addWidget(conversionSumLabel, 4,4);

    // sixth row; entry for units etc
    head->addWidget(unitsMetric, 5,0);
    head->addWidget(unitsImperial, 5,1);
    head->addWidget(conversion, 5,3);
    head->addWidget(conversionSum, 5,4);

    // seventh row; Code label
    head->addWidget(formulaEditLabel, 6,0);

    // 8 - 16 row; program editor
    head->addWidget(formulaEdit, 7,0, 7, 5);
    head->addWidget(errors, 15,0,1,5);

    // 17th row; labels for estimates
    head->addWidget(test, 16, 0);
    head->addWidget(metric, 16, 1);
    head->addWidget(imperial, 16, 2);

    // 18th row; eval values and pushbuttons
    head->addWidget(eval, 17,0);
    head->addWidget(mValue, 17,1);
    head->addWidget(iValue, 17,2);
    head->addWidget(cancelButton, 17,3);
    head->addWidget(okButton, 17,4);

    head->setSpacing(5 *dpiXFactor);
    head->setColumnStretch(0, 15);
    head->setColumnStretch(1, 20);
    head->setColumnStretch(2, 15);
    head->setColumnStretch(3, 20);
    head->setColumnStretch(4, 20);

    mainLayout->addLayout(head);

    connect(symbol, SIGNAL(textChanged(const QString &)), SLOT(enableOk()));
    connect(name, SIGNAL(textChanged(const QString &)), SLOT(enableOk()));

    connect(test, SIGNAL(clicked()), this, SLOT(refreshStats()));
    connect(context, SIGNAL(rideSelected(RideItem*)), this, SLOT(refreshStats()));
    connect (cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect (okButton, SIGNAL(clicked()), this, SLOT(okClicked()));

    // initialize button state
    enableOk();
}

void
EditUserMetricDialog::setErrors(QStringList &list)
{
    errors->setText(list.join(";"));
}

void
EditUserMetricDialog::enableOk()
{
    // Check symbol and name are non-empty, and "_" is not used in name.
    okButton->setEnabled(!symbol->text().isEmpty() && !name->text().isEmpty() && !name->text().contains("_"));
}

bool
EditUserMetricDialog::validSettings()
{
    // user metrics are silently discarded if the symbol is already in use
    const RideMetric* metric = RideMetricFactory::instance().rideMetric(symbol->text());
    if (metric && !metric->isUser()) {
        QMessageBox::critical(this, tr("User Metric"), tr("Symbol already in use by a Builtin metric"));
        return false;
    }

    return true;
}

void
EditUserMetricDialog::setSettings(UserMetricSettings &here)
{
    // set settings from editing widgets
    // Get current values
    here.symbol = symbol->text();
    here.name = name->text();
    here.description = description->document()->toPlainText();
    here.unitsMetric = unitsMetric->text();
    here.unitsImperial = unitsImperial->text();
    here.type = type->currentIndex();
    here.conversion = conversion->value();
    here.conversionSum = conversionSum->value();
    here.aggzero = aggzero->isChecked();
    here.istime = istime->isChecked();
    here.precision = precision->value();
    here.program = formulaEdit->document()->toPlainText();
    here.fingerprint = here.symbol + DataFilter::fingerprint(here.program);
}

void
EditUserMetricDialog::okClicked()
{
    // validate input
    if (!validSettings()) return;

    // fetch current state
    setSettings(settings);

    // ok to accept these
    accept();
}

void
EditUserMetricDialog::refreshStats()
{
    // test against currently selected ride
    if (context->rideItem() == NULL || context->rideItem()->ride() == NULL) return;

    // get current state
    UserMetricSettings here;
    setSettings(here);

    // Build a metric
    UserMetric test(context, here);

    // no spec and no deps, pass empty on stack
    test.compute(context->rideItem(), Specification(), QHash<QString,RideMetric*>());

    // get the value out
    mValue->setText(test.toString(true));
    iValue->setText(test.toString(false));
}
