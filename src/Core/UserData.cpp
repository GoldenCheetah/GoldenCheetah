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

#include "UserData.h"

#include "RideNavigator.h"
#include "Tab.h"
#include "HelpWhatsThis.h"
#include "Utils.h"

#include <QTextEdit> // for parsing trademark symbols (!)

UserData::UserData() 
    : name(""), units(""), formula(""), zstring(""), color(QColor(0,0,0)), rideItem(NULL)
{
}

UserData::UserData(QString settings) 
    : name(""), units(""), formula(""), zstring(""), color(QColor(0,0,0)), rideItem(NULL)
{
    // and apply settings
    setSettings(settings);
}

UserData::UserData(QString name, QString units, QString formula, QString zstring, QColor color)
    : name(name), units(units), formula(formula), zstring(zstring), color(color), rideItem(NULL)
{
}

UserData::~UserData()
{
}

//
// User dialogs to maintain settings
//
bool
UserData::edit()
{
    // settings...
    return false; //XXX
}

static bool insensitiveLessThan(const QString &a, const QString &b)
{
    return a.toLower() < b.toLower();
}

bool
UserData::isEmpty()
{
    foreach(double v, vector)
        if (v != RideFile::NA && v != RideFile::NIL)
            return false;
    return true;
}

EditUserDataDialog::EditUserDataDialog(Context *context, UserData *here) :
    QDialog(context->mainWindow, Qt::Dialog), context(context), here(here)
{
    setWindowTitle(tr("User Data Series"));

    //XXX return to this later
    HelpWhatsThis *help = new HelpWhatsThis(this);
    this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartTrends_MetricTrends_User_Data));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // name and units
    nameEdit = new QLineEdit(this);
    nameEdit->setText(here->name);
    unitsEdit = new QLineEdit(this);
    unitsEdit->setText(here->units);
    zstringEdit = new QLineEdit(this);
    zstringEdit->setText(here->zstring);

    // formula editor
    formulaEdit = new DataFilterEdit(this, context);
    QFont courier("Courier", QFont().pointSize());
    QFontMetrics fm(courier);
    formulaEdit->setFont(courier);
    formulaEdit->setTabStopWidth(4 * fm.width(' ')); // 4 char tabstop
    formulaEdit->setText(here->formula);
    if (here->formula == "") {
        // lets put a template in there
        here->formula = tr("# type in a formula to use\n" 
                                   "# for e.g. BikeStress / Duration\n"
                                   "# as you type the available metrics\n"
                                   "# will be offered by autocomplete\n");
    }
    formulaEdit->setText(here->formula);

    // get suitably formated list XXX too much cut and paste
    // should be done in formula completer !
    QList<QString> list;
    QString last;
    SpecialFields sp;

    // get sorted list
    QStringList names = context->tab->rideNavigator()->logicalHeadings;

    // start with just a list of functions
    list = DataFilter::builtins();

    // ridefile data series symbols
    list += RideFile::symbols();

    // add special functions (older code needs fixing !)
    list << "config(cranklength)";
    list << "config(cp)";
    list << "config(ftp)";
    list << "config(w')";
    list << "config(pmax)";
    list << "config(cv)";
    list << "config(scv)";
    list << "config(height)";
    list << "config(weight)";
    list << "config(lthr)";
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
    list << "RECINTSECS";
    list << "NA";

    qSort(names.begin(), names.end(), insensitiveLessThan);

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

    // color
    seriesColor = new QPushButton(this);
    color = here->color;
    setButtonIcon(color);

    // Widgets
    QGridLayout *widgets = new QGridLayout;
    widgets->addWidget(new QLabel(tr("Name")), 0, 0);
    widgets->addWidget(nameEdit, 0, 1);
    widgets->addWidget(new QLabel(tr("Units")), 1, 0);
    widgets->addWidget(unitsEdit, 1, 1);
    widgets->addWidget(new QLabel(tr("Formula")), 2, 0, Qt::AlignLeft|Qt::AlignTop);
    widgets->addWidget(formulaEdit, 2, 1);
    widgets->addWidget(new QLabel(tr("Zone string")), 3, 0, Qt::AlignLeft|Qt::AlignTop);
    widgets->addWidget(zstringEdit, 3, 1);
    widgets->addWidget(new QLabel(tr("Color")), 4, 0);
    widgets->addWidget(seriesColor, 4, 1);
    mainLayout->addLayout(widgets);
    widgets->setColumnStretch(1,100);
    widgets->setRowStretch(2,100);

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
    connect(seriesColor, SIGNAL(clicked()), this, SLOT(colorClicked()));
}

void
EditUserDataDialog::applyClicked()
{
    here->color = color;
    here->units = unitsEdit->text();
    here->name = nameEdit->text();
    here->formula = formulaEdit->toPlainText();
    here->zstring = zstringEdit->text();
    accept();
}

void
EditUserDataDialog::cancelClicked()
{
    reject();
}

void
EditUserDataDialog::colorClicked()
{
    QColorDialog picker(context->mainWindow);
    picker.setCurrentColor(color);

    // don't use native dialog, since there is a nasty bug causing focus loss
    // see https://bugreports.qt-project.org/browse/QTBUG-14889
    QColor newcolor = picker.getColor(color, this, tr("Choose Metric Color"), QColorDialog::DontUseNativeDialog);

    if (newcolor.isValid()) {
        setButtonIcon(color=newcolor);
    }
}

void
EditUserDataDialog::setButtonIcon(QColor color)
{

    // create an icon
    QPixmap pix(24, 24);
    QPainter painter(&pix);
    if (color.isValid()) {
    painter.setPen(Qt::gray);
    painter.setBrush(QBrush(color));
    painter.drawRect(0, 0, 24, 24);
    }
    QIcon icon;
    icon.addPixmap(pix);
    seriesColor->setIcon(icon);
    seriesColor->setContentsMargins(2,2,2,2);
    seriesColor->setFixedWidth(34);
}

// output a snippet
QString
UserData::settings() const
{
    QString returning;
    returning = "<userdata name=\"" + Utils::xmlprotect(name) + "\" units=\"" +  Utils::xmlprotect(units)+ "\"";
    returning += " color=\""+ color.name() + "\" zstring=\"" + Utils::xmlprotect(zstring) + "\">";
    returning += Utils::xmlprotect(formula);
    returning += "</userdata>";

    // xml snippet
    return returning;

}

// read in a snippet
void
UserData::setSettings(QString settings)
{
    // need to parse the user data xml snipper
    // via xml parser, which is a little over the
    // top but better to keep this stuff as text
    // to avoid the nasty place we ended up in
    // with the LTMSettings structure (yuck!)

    // setup the handler
    QXmlInputSource source;
    source.setData(settings);
    QXmlSimpleReader xmlReader;
    UserDataParser handler(this);
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);

    // parse and instantiate the charts
    xmlReader.parse(source);
}

//
// view layout parser - reads in athletehome/xxx-layout.xml
//

bool UserDataParser::startElement( const QString&, const QString&, const QString &, const QXmlAttributes &attrs )
{
    // get the attributes; color, name, units
    for(int i=0; i<attrs.count(); i++) {
        // only 3 attributes for now
        if (attrs.qName(i) == "color") here->color = QColor(attrs.value(i));
        if (attrs.qName(i) == "name")  here->name  = Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "zstring") here->zstring  = Utils::unprotect(attrs.value(i));
        if (attrs.qName(i) == "units") here->units = Utils::unprotect(attrs.value(i));
    }
    return true;
}

bool UserDataParser::endElement( const QString&, const QString&, const QString &) { return true; }
bool UserDataParser::characters( const QString&chrs) { here->formula += chrs; return true; }
bool UserDataParser::startDocument() { here->formula.clear(); return true; }
bool UserDataParser::endDocument() { here->formula = Utils::unprotect(here->formula); return true; }

// set / get the current ride item
RideItem*
UserData::getRideItem() const
{
    return rideItem;
}

// set ride item and therefore set the data
void
UserData::setRideItem(RideItem*m)
{
    rideItem = m;

    // clear what we got
    vector.clear();

    // if real ..
    if (rideItem) {

        // parse formula
        DataFilter parser(this, rideItem->context, formula);

        // is it cached ?
        vector = rideItem->userCache.value(parser.signature(), QVector<double>());

        if (vector.count() == 0 && rideItem->ride()) {

            // run through each sample and create an equivalent
            foreach(RideFilePoint *p, rideItem->ride()->dataPoints()) {
                Result res = parser.evaluate(rideItem, p);
                vector << res.number;
            }

            // cache for next time !
            rideItem->userCache.insert(parser.signature(), vector);
        }
    }
}
