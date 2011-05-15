/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#include "RideMetadata.h"
#include "SpecialFields.h"

#include "MainWindow.h"
#include "RideSummaryWindow.h"
#include "Settings.h"

#include <QXmlDefaultHandler>
#include <QtGui>

// shorthand when looking up our ride via  Q_PROPERTY
#define ourRideItem meta->property("ride").value<RideItem*>()

/*----------------------------------------------------------------------
 * Master widget for Metadata Entry "on" RideSummaryWindow
 *--------------------------------------------------------------------*/
RideMetadata::RideMetadata(MainWindow *parent) : QWidget(parent), main(parent)
{
    _ride = _connected = NULL;

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);

    QPalette palette;
    palette.setBrush(QPalette::Background, Qt::NoBrush);

    setAutoFillBackground(false);
    setPalette(palette);

    // setup the tabs widget
    tabs = new QTabWidget(this);
    tabs->setMovable(true);
    tabs->setPalette(palette);
    tabs->setAutoFillBackground(false);

    mainLayout->addWidget(tabs);

    // read in metadata.xml and setup the tabs etc
    qRegisterMetaType<RideItem*>("ride"); // XXX we're first here... bit of a hack
    configUpdate();

    // watch for config changes
    connect(main, SIGNAL(configChanged()), this, SLOT(configUpdate()));
}

void
RideMetadata::setRideItem(RideItem *ride)
{
    static QPointer<RideItem> _connected = NULL;
    if (_connected) {
        disconnect(_connected, SIGNAL(rideMetadataChanged()), this, SLOT(metadataChanged()));
    }
    _connected=_ride=ride;

    if (ride) {
        connect (_connected, SIGNAL(rideMetadataChanged()), this, SLOT(metadataChanged()));
        metadataChanged();
    }
}

RideItem*
RideMetadata::rideItem() const
{
    return _ride;
}

void
RideMetadata::metadataChanged()
{
    // loop through each tab
    QMapIterator<QString, Form*> d(tabList);
    d.toFront();
    while (d.hasNext()) {
       d.next();
       foreach(FormField *field, d.value()->fields) // keep track so we can destroy
          field->metadataChanged();
    }
}

void
RideMetadata::configUpdate()
{
    // read metadata.xml
    QString filename = main->home.absolutePath()+"/metadata.xml";
    if (!QFile(filename).exists()) filename = ":/xml/metadata.xml";
    readXML(filename, keywordDefinitions, fieldDefinitions);

    // wipe existing tabs
    QMapIterator<QString, Form*> d(tabList);
    d.toFront();
    while (d.hasNext()) {
           d.next();
           tabs->removeTab(tabs->indexOf(d.value()));
           delete d.value();
    }
    tabList.clear();

    // create forms and populate with fields
    foreach(FieldDefinition field, fieldDefinitions) {

        if (field.tab == "") continue; // not to be shown!

        Form *form;
        if ((form = tabList.value(field.tab, NULL)) == NULL) {
            form = new Form(this);
            tabs->addTab(form, field.tab);
            tabList.insert(field.tab, form);
        }
        form->addField(field);
    }

    // get forms arranged
    QMapIterator<QString, Form*> i(tabList);
    i.toFront();
    while (i.hasNext()) {
           i.next();
           i.value()->arrange();
    }

    // when constructing we have not registered
    // the properties nor selected a ride
    metadataChanged(); // re-read the values!
}

/*----------------------------------------------------------------------
 * Forms (one per tab)
 *--------------------------------------------------------------------*/
Form::Form(RideMetadata *meta) : meta(meta)
{
    QPalette palette;
    palette.setBrush(QPalette::Background, Qt::NoBrush);

    setPalette(palette);
    setAutoFillBackground(false);

    palette.setBrush(QPalette::Background, Qt::NoBrush);
    contents = new QWidget;
    contents->setPalette(palette);
    contents->setAutoFillBackground(false);

    QVBoxLayout *mainLayout = new QVBoxLayout(contents);
    hlayout = new QHBoxLayout;
    vlayout1 = vlayout2 = NULL;
    grid1 = grid2 = NULL;
    mainLayout->addLayout(hlayout);
    //mainLayout->addStretch();
    contents->setLayout(mainLayout);

    setFrameStyle(QFrame::NoFrame);
    setWidgetResizable(true);
    setWidget(contents);
}

Form::~Form()
{
    // wipe the fields
    foreach (FormField *current, fields) {
        delete current;
    }
    // wipe away the layouts
    foreach (QHBoxLayout *h, overrides) delete h;
    if (grid1) delete grid1;
    if (grid2) delete grid2;
    if (vlayout1) delete vlayout1;
    if (vlayout2) delete vlayout2;
    delete hlayout;
    delete contents;
}

void
Form::arrange()
{
    QGridLayout *here;

    int x=0;
    int y=0;

    // special case -- a textbox and its the only field on the tab needs no label
    //                 this is how the "Notes" tab is created
    if (fields.count() == 1 && fields[0]->definition.type == FIELD_TEXTBOX) {
        hlayout->addWidget(fields[0]->widget, 0, 0);
        return;
    } else {
        vlayout1 = new QVBoxLayout;
        here = grid1 = new QGridLayout;
        vlayout1->addLayout(here);
        vlayout1->addStretch();
        hlayout->addLayout(vlayout1);
    }

    int rows = ceil(fields.count() / 2.0);
    if (rows < 5) rows = 5; // minimum of 5 rows seems pleasing


    for (int i=0; i<fields.count(); i++) {

        if (y >= rows) {
            x+=1;
            y=0;

            vlayout2 = new QVBoxLayout;
            here = grid2 = new QGridLayout;
            vlayout2->addLayout(here);
            vlayout2->addStretch();
            hlayout->addLayout(vlayout2);
        }

        here->setColumnStretch(0,0);
        here->setColumnStretch(1,3);

        Qt::Alignment labelalignment = Qt::AlignLeft|Qt::AlignTop;
        Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignTop;

        if (fields[i]->definition.type < FIELD_SHORTTEXT) alignment = 0; // text types

        here->addWidget(fields[i]->label, y, 0, labelalignment);

        if (sp.isMetric(fields[i]->definition.name)) {
            QHBoxLayout *override = new QHBoxLayout;
            overrides.append(override);

            override->setContentsMargins(0,0,0,0);
            override->addWidget(fields[i]->enabled);
            override->addWidget(fields[i]->widget);
            here->addLayout(override, y, 1, alignment);

        //} else  if (fields[i]->definition.type == FIELD_TEXTBOX) {
        //    here->addWidget(fields[i]->widget, ++y, 0, 2, 2, alignment);
        //    y++;
        } else {
            here->addWidget(fields[i]->widget, y, 1, alignment);
        }
        y++;
    }
}

/*----------------------------------------------------------------------
 * Form fields
 *--------------------------------------------------------------------*/
FormField::FormField(FieldDefinition field, RideMetadata *meta) : definition(field), meta(meta), active(false)
{
    QFont font;
    font.setPointSize(font.pointSize()-2);

    QString units;

    if (sp.isMetric(field.name)) {
        field.type = FIELD_DOUBLE; // whatever they say, we want a double!
        QVariant unit = appsettings->value(this, GC_UNIT);
        bool useMetricUnits = (unit.toString() == "Metric");
        units = sp.rideMetric(field.name)->units(useMetricUnits);
        if (units != "") units = QString(" (%1)").arg(units);
    }

    label = new QLabel(QString("%1%2").arg(field.name).arg(units), this);
    //label->setFont(font);
    //label->setFixedHeight(18);

    switch(field.type) {

    case FIELD_TEXT : // text
    case FIELD_SHORTTEXT : // shorttext
        if (field.name == "Keywords")
            widget = new KeywordField(this);
        else
            widget = new QLineEdit(this);
        //widget->setFixedHeight(18);
        connect (widget, SIGNAL(textChanged(const QString&)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_TEXTBOX : // textbox
        widget = new QTextEdit(this);
        if (field.name == "Change History") {
            dynamic_cast<QTextEdit*>(widget)->setReadOnly(true);
            // pick up when ride saved - since it gets updated then
            // XXX ? connect (main, SIGNAL(rideClean()), this, SLOT(rideSelected()));
        } else {
            connect (widget, SIGNAL(textChanged()), this, SLOT(editFinished()));
        }
        break;

    case FIELD_INTEGER : // integer
        widget = new QSpinBox(this);
        //widget->setFixedHeight(18);
        ((QSpinBox*)widget)->setSingleStep(1);
        ((QSpinBox*)widget)->setMinimum(-99999999);
        ((QSpinBox*)widget)->setMaximum(99999999);
        connect (widget, SIGNAL(valueChanged(int)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_DOUBLE : // double
        widget = new QDoubleSpinBox(this);
        //widget->setFixedHeight(18);
        ((QDoubleSpinBox*)widget)->setSingleStep(0.01);
        ((QDoubleSpinBox*)widget)->setMaximum(999999.99);
        if (sp.isMetric(field.name)) {
            enabled = new QCheckBox(this);
            connect(enabled, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
        }
        connect (widget, SIGNAL(valueChanged(double)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_DATE : // date
        widget = new QDateEdit(this);
        //widget->setFixedHeight(18);
        connect (widget, SIGNAL(dateChanged(const QDate)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_TIME : // time
        widget = new QTimeEdit(this);
        //widget->setFixedHeight(18);
        ((QTimeEdit*)widget)->setDisplayFormat("hh:mm:ss AP");
        connect (widget, SIGNAL(timeChanged(const QTime)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;
    }
    //widget->setFont(font);
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));
}

FormField::~FormField()
{
    delete label;

    switch (definition.type) {
        case FIELD_TEXT:
        case FIELD_SHORTTEXT: if (definition.name == "Keywords")
                    delete (KeywordField*)widget;
                else
                    delete (QLineEdit*)widget;
                break;
        case FIELD_TEXTBOX : if (definition.name == "Summary")
                                 delete ((RideSummaryWindow *)widget);
                             else
                                 delete ((QTextEdit*)widget);
                             break;
        case FIELD_INTEGER : delete ((QSpinBox*)widget); break;
        case FIELD_DOUBLE : delete ((QDoubleSpinBox*)widget); break;
        case FIELD_DATE : delete ((QDateEdit*)widget); break;
        case FIELD_TIME : delete ((QTimeEdit*)widget); break;
    }
    if (sp.isMetric(definition.name)) delete enabled;
}

void
FormField::dataChanged()
{
    if (active) return;
    edited = true;
}

void
FormField::editFinished()
{
    if (active || ourRideItem == NULL ||
        (edited == false && definition.type != FIELD_TEXTBOX) ||
        ourRideItem == NULL) return;

    // get values from the widgets
    QString text;

    // get the current value into a string
    switch (definition.type) {
    case FIELD_TEXT :
    case FIELD_SHORTTEXT : if (definition.name == "Keywords")
                text = ((KeywordField*)widget)->text();
             else
                text = ((QLineEdit*)widget)->text();
             break;
    case FIELD_TEXTBOX :
        {
            text = ((QTextEdit*)widget)->document()->toPlainText();
            break;
        }

    case FIELD_INTEGER : text = QString("%1").arg(((QSpinBox*)widget)->value()); break;
    case FIELD_DOUBLE : text = QString("%1").arg(((QDoubleSpinBox*)widget)->value()); break;
    case FIELD_DATE : text = ((QDateEdit*)widget)->date().toString("dd.MM.yyyy"); break;
    case FIELD_TIME : text = ((QTimeEdit*)widget)->time().toString("hh:mm:ss.zzz"); break;
    }


    // Update special field
    if (definition.name == "Device") {
        ourRideItem->ride()->setDeviceType(text);
        ourRideItem->notifyRideMetadataChanged();
    } else if (definition.name == "Identifier") {
        ourRideItem->ride()->setId(text);
        ourRideItem->notifyRideMetadataChanged();
    } else if (definition.name == "Recording Interval") {
        ourRideItem->ride()->setRecIntSecs(text.toDouble());
        ourRideItem->notifyRideMetadataChanged();
    } else if (definition.name == "Start Date") {
        QDateTime current = ourRideItem->ride()->startTime();
        QDate date(/* year*/text.mid(6,4).toInt(),
                   /* month */text.mid(3,2).toInt(),
                   /* day */text.mid(0,2).toInt());
        QDateTime update = QDateTime(date, current.time());
        ourRideItem->setStartTime(update);
        ourRideItem->notifyRideMetadataChanged();
    } else if (definition.name == "Start Time") {
        QDateTime current = ourRideItem->ride()->startTime();
        QTime time(/* hours*/ text.mid(0,2).toInt(),
                   /* minutes */ text.mid(3,2).toInt(),
                   /* seconds */ text.mid(6,2).toInt(),
                   /* milliseconds */ text.mid(9,3).toInt());
        QDateTime update = QDateTime(current.date(), time);
        ourRideItem->setStartTime(update);
        ourRideItem->notifyRideMetadataChanged();
    } else if (definition.name != "Summary") {
        if (sp.isMetric(definition.name) && enabled->isChecked()) {

            QVariant unit = appsettings->value(this, GC_UNIT);
            bool useMetricUnits = (unit.toString() == "Metric");

            // convert from imperial to metric if needed
            if (!useMetricUnits) {
                double value = text.toDouble() * (1/ sp.rideMetric(definition.name)->conversion());
                text = QString("%1").arg(value);
            }

            // update metric override QMap!
            QMap<QString,QString> override;
            override.insert("value", text);
            ourRideItem->ride()->metricOverrides.insert(sp.metricSymbol(definition.name), override);

            // get widgets updated with new override
            ourRideItem->notifyRideMetadataChanged();
        } else {
            // just update the tags QMap!
            ourRideItem->ride()->setTag(definition.name, text);
        }
    }

    // Construct the summary text used on the calendar
    QString calendarText;
    foreach (FieldDefinition field, meta->getFields()) {
        if (field.diary == true) {
            calendarText += QString("%1\n")
                    .arg(ourRideItem->ride()->getTag(field.name, ""));
        }
    }
    ourRideItem->ride()->setTag("Calendar Text", calendarText);

    // rideFile is now dirty!
    ourRideItem->setDirty(true);
}

void
FormField::stateChanged(int state)
{
    if (active) return; // being updated programmatically

    widget->setEnabled(state ? true : false);
    widget->setHidden(state ? false : true);

    // remove from overrides if neccessary
    QMap<QString,QString> override;
    if (ourRideItem && ourRideItem->ride())
        override  = ourRideItem->ride()->metricOverrides.value
                                        (sp.metricSymbol(definition.name));

    // setup initial override value
    if (state) {

        // clear and reset override value for this metric
        override.insert("value", QString("%1").arg(0.0)); // add metric value
        ((QDoubleSpinBox *)widget)->setValue(0.0);

    } else if (override.contains("value")) // clear override value
         override.remove("value");

    if (ourRideItem && ourRideItem->ride()) {
        // update overrides for this metric in the main QMap
        ourRideItem->ride()->metricOverrides.insert(sp.metricSymbol(definition.name), override);

        // rideFile is now dirty!
        ourRideItem->setDirty(true);

        // get refresh done, coz overrides state has changed
        ourRideItem->notifyRideMetadataChanged();
    }
}

void
FormField::metadataChanged()
{
    active = true;
    edited = false;
    QString value;

    if (ourRideItem && ourRideItem->ride()) {

        // Handle "Special" fields
        if (definition.name == "Device") value = ourRideItem->ride()->deviceType();
        else if (definition.name == "Recording Interval") value = QString("%1").arg(ourRideItem->ride()->recIntSecs());
        else if (definition.name == "Start Date") value = ourRideItem->ride()->startTime().date().toString("dd.MM.yyyy");
        else if (definition.name == "Start Time") value = ourRideItem->ride()->startTime().time().toString("hh:mm:ss.zzz");
        else if (definition.name == "Identifier") value = ourRideItem->ride()->id();
        else {
            if (sp.isMetric(definition.name)) {
                //  get from metric overrides
                const QMap<QString,QString> override =
                      ourRideItem->ride()->metricOverrides.value(sp.metricSymbol(definition.name));
                if (override.contains("value")) {
                    enabled->setChecked(true);
                    widget->setEnabled(true);
                    widget->setHidden(false);
                    value = override.value("value");

                    // does it need conversion from metric?
                    if (sp.rideMetric(definition.name)->conversion() != 1.0) {
                        QVariant unit = appsettings->value(this, GC_UNIT);
                        bool useMetricUnits = (unit.toString() == "Metric");

                        // do we want imperial?
                        if (!useMetricUnits) {
                            double newvalue = value.toDouble() * sp.rideMetric(definition.name)->conversion();
                            value = QString("%1").arg(newvalue);
                        }
                    }
                } else {
                    value = "0.0";
                    enabled->setChecked(false);
                    widget->setEnabled(false);
                    widget->setHidden(true);
                }
            } else {
                value = ourRideItem->ride()->getTag(definition.name, "");
            }
        }
    } else {
        value ="";
    }

    switch (definition.type) {
    case FIELD_TEXT : // text
    case FIELD_SHORTTEXT : // shorttext
        if (definition.name == "Keywords")
            ((KeywordField*)widget)->setText(value);
        else
            ((QLineEdit*)widget)->setText(value);
        break;

    case FIELD_TEXTBOX : // textbox
        if (definition.name != "Summary")
            ((QTextEdit*)widget)->setText(value);
        break;

    case FIELD_INTEGER : // integer
        ((QSpinBox*)widget)->setValue(value.toInt());
        break;

    case FIELD_DOUBLE : // double
        ((QDoubleSpinBox*)widget)->setValue(value.toDouble());
        break;

    case FIELD_DATE : // date
        {
        if (value == "") value = "          ";
        QDate date(/* year*/value.mid(6,4).toInt(),
                   /* month */value.mid(3,2).toInt(),
                   /* day */value.mid(0,2).toInt());
        ((QDateEdit*)widget)->setDate(date);
        }
        break;

    case FIELD_TIME : // time
        {
        if (value == "") value = "            ";
        QTime time(/* hours*/ value.mid(0,2).toInt(),
                   /* minutes */ value.mid(3,2).toInt(),
                   /* seconds */ value.mid(6,2).toInt(),
                   /* milliseconds */ value.mid(9,3).toInt());
        ((QTimeEdit*)widget)->setTime(time);
        }
        break;
    }
    active = false;
}

/*----------------------------------------------------------------------
 * Read / Write metadata.xml file
 *--------------------------------------------------------------------*/

// static helper to protect special xml characters
// ideally we would use XMLwriter to do this but
// the file format is trivial and this implementation
// is easier to follow and modify... for now.
static QString xmlprotect(QString string)
{
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    QString s = string;
    s.replace( tm, "&#8482;" );
    s.replace( "&", "&amp;" );
    s.replace( ">", "&gt;" );
    s.replace( "<", "&lt;" );
    s.replace( "\"", "&quot;" );
    s.replace( "\'", "&apos;" );
    return s;
}

void
RideMetadata::serialize(QString filename, QList<KeywordDefinition>keywordDefinitions, QList<FieldDefinition>fieldDefinitions)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);

    // begin document
    out << "<metadata>\n";

    //
    // First we write out the keywords
    //
    out << "\t<keywords>\n";
    // write out to file
    foreach (KeywordDefinition keyword, keywordDefinitions) {
        // chart name
        out<<QString("\t\t<keyword>\n");
        out<<QString("\t\t\t<keywordname>\"%1\"</keywordname>\n").arg(xmlprotect(keyword.name));
        out<<QString("\t\t\t<keywordcolor red=\"%1\" green=\"%3\" blue=\"%4\"></keywordcolor>\n")
                        .arg(keyword.color.red())
                        .arg(keyword.color.green())
                        .arg(keyword.color.blue());

        foreach (QString token, keyword.tokens)
            out<<QString("\t\t\t<keywordtoken>\"%1\"</keywordtoken>\n").arg(xmlprotect(token));

        out<<QString("\t\t</keyword>\n");
    }
    out <<"\t</keywords>\n";

    //
    // Last we write out the field definitions
    //
    out << "\t<fields>\n";
    // write out to file
    foreach (FieldDefinition field, fieldDefinitions) {
        // chart name
        out<<QString("\t\t<field>\n");
        out<<QString("\t\t\t<fieldtab>\"%1\"</fieldtab>\n").arg(xmlprotect(field.tab));
        out<<QString("\t\t\t<fieldname>\"%1\"</fieldname>\n").arg(xmlprotect(field.name));
        out<<QString("\t\t\t<fieldtype>%1</fieldtype>\n").arg(field.type);
        out<<QString("\t\t\t<fielddiary>%1</fielddiary>\n").arg(field.diary ? 1 : 0);
        out<<QString("\t\t</field>\n");
    }
    out <<"\t</fields>\n";

    // end document
    out << "</metadata>\n";

    // close file
    file.close();
}

static QString unprotect(QString buffer)
{
    // get local TM character code
    QTextEdit trademark("&#8482;"); // process html encoding of(TM)
    QString tm = trademark.toPlainText();

    // remove quotes
    QString t = buffer.trimmed();
    QString s = t.mid(1,t.length()-2);

    // replace html (TM) with local TM character
    s.replace( "&#8482;", tm );

    // html special chars are automatically handled
    // other special characters will not work
    // cross-platform but will work locally, so not a biggie
    // i.e. if thedefault charts.xml has a special character
    // in it it should be added here
    return s;
}

void
RideMetadata::readXML(QString filename, QList<KeywordDefinition>&keywordDefinitions, QList<FieldDefinition>&fieldDefinitions)
{
    QFile metadataFile(filename);
    QXmlInputSource source( &metadataFile );
    QXmlSimpleReader xmlReader;
    MetadataXMLParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse( source );

    keywordDefinitions = handler.getKeywords();
    fieldDefinitions = handler.getFields();

    // now auto append special fields, in case
    // the user wiped them or we have introduced
    // them in this release. This is to ensure
    // they get written to metricDB
    bool hasCalendarText = false;
    foreach (FieldDefinition field, fieldDefinitions) {
        if (field.name == "Calendar Text") {
            hasCalendarText = true;
        }
        // other fields here...
    }
    if (!hasCalendarText) {
        FieldDefinition add;
        add.name = "Calendar Text";
        add.type = 1;
        add.diary = false;
        add.tab = "";

        fieldDefinitions.append(add);
    }
}

// to see the format of the charts.xml file, look at the serialize()
// function at the bottom of this source file.
bool MetadataXMLParser::endElement( const QString&, const QString&, const QString &qName)
{
    //
    // Single Attribute elements
    //
    if(qName == "keywordname") keyword.name = unprotect(buffer);
    else if(qName == "keywordtoken") keyword.tokens << unprotect(buffer);
    else if(qName == "keywordcolor") {
            // the r,g,b values are in red="xx",green="xx" and blue="xx" attributes
            // of this element and captured in startelement below
            keyword.color = QColor(red,green,blue);
    }
    else if(qName == "fieldtab") field.tab = unprotect(buffer);
    else if(qName == "fieldname") field.name =  unprotect(buffer);
    else if(qName == "fieldtype") {
        field.type = buffer.trimmed().toInt();
        if (field.tab != "" && field.type < 3 && field.name != "Filename" &&
            field.name != "Change History") field.diary = true; // default!
    } else if (qName == "fielddiary") field.diary = (buffer.trimmed().toInt() != 0);

    //
    // Complex Elements
    //
    else if(qName == "keyword") // <keyword></keyword> block
        keywordDefinitions.append(keyword);
    else if (qName == "field") // <field></field> block
        fieldDefinitions.append(field);

    return TRUE;
}

bool MetadataXMLParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs )
{
    buffer.clear();
    if(name == "keywords")
        keywordDefinitions.clear();
    else if (name == "fields")
        fieldDefinitions.clear();
    else if (name == "field")
        field = FieldDefinition();
    else if (name == "keyword")
        keyword = KeywordDefinition();
    else if (name == "keywordcolor") {

        // red="x" green="x" blue="x" attributes for pen/brush color
        for(int i=0; i<attrs.count(); i++) {
            if (attrs.qName(i) == "red") red=attrs.value(i).toInt();
            if (attrs.qName(i) == "green") green=attrs.value(i).toInt();
            if (attrs.qName(i) == "blue") blue=attrs.value(i).toInt();
        }
    }

    return TRUE;
}

bool MetadataXMLParser::characters( const QString& str )
{
    buffer += str;
    return TRUE;
}

bool MetadataXMLParser::endDocument()
{
    return TRUE;
}

/*----------------------------------------------------------------------
 * Keyword completer
 *--------------------------------------------------------------------*/

KeywordField::KeywordField(QWidget *parent) : QLineEdit(parent)
{
//XXX The completer code needs to be implemented separately
//    and take into account the constraints of the current
//    QCompleter implementation which assumes the completion
//    is either for the entire contents or is a directory path
}

void
KeywordField::textUpdated(const QString &/*sofar*/)
{
}

void
KeywordField::completeText(QString /*prefix*/, QString /*token*/)
{
}

void
KeywordField::completerActive(const QString &/*p*/)
{
}

void
KeywordCompleter::textUpdated(const QString &/*sofar*/)
{
}

// split into comma separated tokens
QStringList
KeywordCompleter::splitPath(const QString &sofar) const
{
    QStringList returnList;

    // nothing to complete
    if (sofar == "" || sofar.endsWith(",")) return returnList;

    QStringList tokens = sofar.split(",", QString::SkipEmptyParts);
    if (tokens.count() == 0) return returnList; // empty!

    for (int i=tokens.count()-1; i>=0; i--) returnList<<tokens[i];

    // set the completion prefix!
    return returnList;
}
