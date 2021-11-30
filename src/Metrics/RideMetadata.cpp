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
#include "RideCache.h"
#include "RideItem.h"
#include "Context.h"
#include "Athlete.h"
#include "Settings.h"
#include "Colors.h"
#include "Units.h"
#include "AbstractView.h"
#include "HelpWhatsThis.h"
#include "Utils.h"
#include "RideEditor.h"

#include <QXmlDefaultHandler>
#include <QtGui>
#include <QLineEdit>
#include <QStyle>
#include <QStyleFactory>
#include <QSqlQuery>

// shorthand when looking up our ride via  Q_PROPERTY
#define ourRideItem meta->property("ride").value<RideItem*>()

/*----------------------------------------------------------------------
 * Master widget for Metadata Entry
 *--------------------------------------------------------------------*/
RideMetadata::RideMetadata(Context *context, bool singlecolumn) :
    QWidget(context != NULL ? context->mainWindow : NULL), singlecolumn(singlecolumn), context(context)
{

    if (context) {
        _ride = _connected = NULL;

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0,0,0,0);

        HelpWhatsThis *help = new HelpWhatsThis(this);
        this->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_Details));

        // setup the tabs widget
        tabs = new QTabWidget(this);
        tabs->setMovable(false);
        tabs->setContentsMargins(0,0,0,0);

        // better styling on Linux with fusion controls
#ifndef Q_OS_MAC
        QStyle *fusion = QStyleFactory::create(OS_STYLE);
        tabs->setStyle(fusion);
#endif
        mainLayout->addWidget(tabs);

        // read in metadata.xml and setup the tabs etc
        qRegisterMetaType<RideItem*>("ride");

        extraForm = new Form(this);
        editor = new RideEditor(context);

        // Extra tab is expensive to update so we only update if it
        // is visible. In this case we need to trigger refresh when the
        // tab is selected -or- when the ride changes. Below is for tab.
        connect(tabs, SIGNAL(currentChanged(int)), this, SLOT(setExtraTab()));
        connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    } else {

        // we are not a real widget
        hide();
    }

    // read config in
    configChanged(CONFIG_FIELDS | CONFIG_APPEARANCE);

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
        setExtraTab();
        metadataChanged();
    }
    editor->rideSelected(ride);
}

void
RideMetadata::warnDateTime(QDateTime datetime)
{
    if (rideItem() == NULL) return;

    // see if there is a ride with this date/time already?
    // Check if an existing ride has the same starttime
    QChar zero = QLatin1Char('0');
    QString targetnosuffix = QString ("%1_%2_%3_%4_%5_%6")
                           .arg(datetime.date().year(), 4, 10, zero)
                           .arg(datetime.date().month(), 2, 10, zero)
                           .arg(datetime.date().day(), 2, 10, zero)
                           .arg(datetime.time().hour(), 2, 10, zero)
                           .arg(datetime.time().minute(), 2, 10, zero)
                           .arg(datetime.time().second(), 2, 10, zero);

    // now make a regexp for all know ride types
    foreach(QString suffix, RideFileFactory::instance().suffixes()) {

        QString conflict = context->athlete->home->activities().canonicalPath() + "/" + targetnosuffix + "." + suffix;
        if (QFile(conflict).exists() && QFileInfo(conflict).fileName() != rideItem()->fileName) {
            QMessageBox::warning(this, "Date/Time Entry", "An activity already exists with that date/time, if you do not change it then you will overwrite and lose existing data");
            return; // only warn on the first conflict!
        }
    }
}

// the extra tab has all the fields that are in the current
// ride that are not shown on the tabs
void
RideMetadata::setExtraTab()
{
    // wipe out the form & re-initialise -- special case for extras
    extraForm->clear();
    extraDefs.clear();
    extraForm->initialise();

    // no ride available...
    if (!extraForm->isVisible() || !rideItem()) {
        return;
    }

    // look for metadata not in config
    QMapIterator<QString,QString> tags(rideItem()->ride()->tags());
    tags.toFront();
    while (tags.hasNext()) {

        tags.next();

        bool configured = false;
        foreach (FieldDefinition x, fieldDefinitions) {
            if (x.tab != "" && x.name == tags.key()) {
                configured = true;
                break;
            }
        }

        // Try and guess the field type, for now we assume everything
        // is a string; if its short make it a shorttext
        //              if it contains newlines make it a textbox
        //              Otherwise make it a text
        //
        if (!configured) {
            int type;
            if (tags.value().contains("\n")) type = FIELD_TEXTBOX;
            else if (tags.value().length() < 30) type = FIELD_SHORTTEXT;
            else type = FIELD_TEXT;

            extraDefs.append(FieldDefinition("Extra", tags.key(), type, false, QStringList()));
            extraForm->addField(extraDefs[extraDefs.count()-1]);
        }
    }

    // now arrange the form - but only if we have anything to display!
    if (extraForm->fields.count()) {
        // arrange them
        extraForm->arrange();

        // fetch values
        foreach(FormField *field, extraForm->fields) {
            field->metadataChanged();

            // since we show EVERYTHING, don't let the user edit them
            // we might get more selective later?

            if (field->enabled) {
                field->enabled->setEnabled(false);
                field->widget->setEnabled(false);
            } else {
                // set Text Field to 'Read Only' to still enable scrolling,...
                GTextEdit* textEdit = dynamic_cast<GTextEdit*> (field->widget);
                if (textEdit)  textEdit->setReadOnly(true);
                else {
                    QLineEdit* lineEdit = dynamic_cast<QLineEdit*> (field->widget);
                    if (lineEdit) lineEdit->setReadOnly(true);
                    else field->widget->setEnabled(false);

                }
            }
        }
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
RideMetadata::configChanged(qint32)
{
    // read global metadata.xml
    QString filename = QDir(gcroot).absolutePath()+"/metadata.xml";

    // no global metadata.xml, so construct from old athlete specific ones
    // if none of those exist, then drop back to the built in one
    if (QFile(filename).exists()) {

        // read in existing file
        readXML(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);

    } else {

        // prior to v3.6 the metadata configuration was athlete specific
        // this was changed to be global configuration since other global
        // settings are dependant on them, notably user metrics
        bool first=true;
        QStringListIterator i(QDir(gcroot).entryList(QDir::Dirs | QDir::NoDotAndDotDot));
        while (i.hasNext()) {

            QString name = i.next();

            // if there is no metadata.xml it is not an athlete folder
            // or it is one still using default metadata.xml, so skip it
            // since there is no custom configuration to preserve.
            QString filename = gcroot + "/" + name + "/config/metadata.xml";
            if (!QFile(filename).exists()) continue;

            // athlete specific configuration
            QList<KeywordDefinition> a_keywordDefinitions;
            QList<FieldDefinition>   a_fieldDefinitions;
            QList<DefaultDefinition>   a_defaultDefinitions;
            QString a_colorfield;

            // read in athlete specific
            readXML(filename, a_keywordDefinitions, a_fieldDefinitions, a_colorfield, a_defaultDefinitions);

            // first one, just use them all
            if (first) {

                // just adopt all the config for the first athlete found
                keywordDefinitions = a_keywordDefinitions;
                fieldDefinitions = a_fieldDefinitions;
                defaultDefinitions = a_defaultDefinitions;
                colorfield = a_colorfield;

                first = false;

            } else {

                // now we just append any that we don't already have
                // might be a bit awks but at least nothing is lost
                // user can always go an edit them in settings anyway
                foreach(KeywordDefinition x, a_keywordDefinitions) {

                    bool found=false;
                    foreach(KeywordDefinition e, keywordDefinitions) {
                        if (e.name == x.name) found=true;
                    }
                    if (!found) {
                        keywordDefinitions.append(x);
                    }
                }

                foreach(FieldDefinition x, a_fieldDefinitions) {

                    bool found=false;
                    foreach(FieldDefinition e, fieldDefinitions) {
                        if (e.tab == x.tab && e.name == x.name) found=true;
                    }
                    if (!found) {
                        fieldDefinitions.append(x);
                    }
                }

                foreach(DefaultDefinition x, a_defaultDefinitions) {

                    bool found=false;
                    foreach(DefaultDefinition e, defaultDefinitions) {
                        if (e.field == x.field && e.value == x.value) found=true;
                    }
                    if (!found) {
                        defaultDefinitions.append(x);
                    }
                }
            }
        }

        // eek, we didn't find anything - new install with no athletes yet
        if (first == true) {

            // use the default setup
            filename = ":/xml/metadata.xml";
            readXML(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);
        }

        // write out global file now, so we don't keep doing this
        // "upgrade" process every time we open an athlete etc
        filename = QDir(gcroot).absolutePath()+"/metadata.xml";
        serialize(filename, keywordDefinitions, fieldDefinitions, colorfield, defaultDefinitions);
    }

    if (context) { // global doesn't have all the widgets etc
        setProperty("color", GColor(CPLOTBACKGROUND));

        palette = QPalette();

        palette.setColor(QPalette::Window, GColor(CPLOTBACKGROUND));
        palette.setColor(QPalette::Background, GColor(CPLOTBACKGROUND));

        // only change base if moved away from white plots
        // which is a Mac thing
#ifndef Q_OS_MAC
        if (GColor(CPLOTBACKGROUND) != Qt::white)
#endif
        {
            palette.setColor(QPalette::Base, GCColor::alternateColor(GColor(CPLOTBACKGROUND)));
            palette.setColor(QPalette::Window,  GColor(CPLOTBACKGROUND));
        }

        palette.setColor(QPalette::WindowText, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        palette.setColor(QPalette::Text, GCColor::invertColor(GColor(CPLOTBACKGROUND)));
        setPalette(palette);
        tabs->setPalette(palette);

        // use default font
        setFont(QFont());

        // wipe existing tabs
        QMapIterator<QString, Form*> d(tabList);
        d.toFront();
        while (d.hasNext()) {
               d.next();
               tabs->removeTab(tabs->indexOf(d.value()));
               delete d.value();
        }
        tabList.clear();
        formFields.empty();

        // remove editor if it exists
        if (tabs->count() > 0) tabs->removeTab(tabs->count()-1);

        // create forms and populate with fields
        for(int i=0; i<fieldDefinitions.count(); i++) {

            if (fieldDefinitions[i].tab == "") continue; // not to be shown!

            Form *form;
            QString tabName = specialTabs.displayName(fieldDefinitions[i].tab);
            if ((form = tabList.value(tabName, NULL)) == NULL) {
                form = new Form(this);
                tabs->addTab(form, tabName);
                tabList.insert(tabName, form);
            }
            form->addField(fieldDefinitions[i]);
        }

        // get forms arranged
        QMapIterator<QString, Form*> i(tabList);
        i.toFront();
        while (i.hasNext()) {
               i.next();
               i.value()->arrange();
        }

        // add extra back
        tabs->addTab(extraForm, "Extra");

        // set Extra tab for current ride
        setExtraTab();

        // add editor
        tabs->addTab(editor, "Raw Data");

        // tab bar look reasonably modern
        QString styling = QString("QTabWidget { background: %1; }"
                              "QTabWidget::pane { border: 0px; }"
#ifdef Q_OS_MAC
                              "QLineEdit { border: 0px; background-color: %7; color: %6; }"
                              "QDateEdit { border: 0px; background-color: %7; color: %6; }"
                              "QTimeEdit { border: 0px; background-color: %7; color: %6; }"
                              "QSpinBox { border: 0px; background-color: %7; color: %6; }"
                              "QDoubleSpinBox { border: 0px; background-color: %7; color: %6; }"
#endif
                              "QTabBar::tab { background: %1; "
                              "               color: %6; "
                              "               min-width: %5px; "
                              "               padding: %4px; "
                              "               border-top: 0px;"
                              "               border-left: 0px;"
                              "               border-right: 0px;"
                              "               border-bottom: %3px solid %1; } "
                              "QTabBar::tab:selected { border-bottom-right-radius: 0px; border-bottom-left-radius: 0px; border-bottom-color: %2; }"
                             ).arg(GColor(CPLOTBACKGROUND).name())                        // 1 tab background color
                              .arg(GColor(CPLOTMARKER).name())                            // 2 selected bar color
                              .arg(4*dpiYFactor)                                          // 3 selected bar width
                              .arg(2*dpiXFactor)                                          // 4 padding
                              .arg(75*dpiXFactor)                                         // 5 tab minimum width
                              .arg(GCColor::invertColor(GColor(CPLOTBACKGROUND)).name())     // 6 tab text color
#ifdef Q_OS_MAC
                              .arg( GCColor::alternateColor(GColor(CPLOTBACKGROUND)).name()) // 7 lineedit background
#endif
                            ;
        tabs->setStyleSheet(styling);

        metadataChanged(); // re-read the values!
    }
}

void
RideMetadata::addFormField(FormField *f)
{
    formFields.append(f);
}

QVector<FormField *> RideMetadata::getFormFields()
{
    return formFields;
}

// Are there fields enabled for CalendarText?
bool
RideMetadata::hasCalendarText()
{
    foreach (FieldDefinition field, getFields()) {
        if (field.diary) return true;
    }
    return false;
}

// Construct the summary text used on the calendar
QString
RideMetadata::calendarText(RideItem *rideItem)
{
    QString calendarText;

    foreach (FieldDefinition field, getFields()) {
        // "Weight" field is replace by "Athlete Weight" metric
        QString fieldName = (field.name == "Weight") ? "Athlete Weight" :
                                                       field.name;
        QString value;
        if (GlobalContext::context()->specialFields.isMetric(fieldName)) {
            value = rideItem->getStringForSymbol(GlobalContext::context()->specialFields.rideMetric(fieldName)->symbol(), GlobalContext::context()->useMetricUnits);
        } else {
            value = rideItem->getText(fieldName, "");
        }
        calendarText += field.calendarText(value);
    }

    return calendarText;
}

// Sports, as displayed to users
QStringList
RideMetadata::sports()
{
    QStringList sportList;
    foreach (FieldDefinition field, getFields()) {
        if (field.name == "Sport") {
            sportList = field.values;
	    break;
        }
    }

    // Ensure default sport
    bool hasBike = false;
    foreach (QString sport, sportList) {
        if (RideFile::sportTag(sport) == "Bike") hasBike = true;
    }
    if (!hasBike) sportList.prepend(tr("Bike"));

    return sportList;
}

/*----------------------------------------------------------------------
 * Forms (one per tab)
 *--------------------------------------------------------------------*/
Form::Form(RideMetadata *meta) : meta(meta)
{
    initialise();
}

Form::~Form()
{
    clear();
}

void
Form::clear()
{

    // wipe the fields
    foreach (FormField *current, fields) {
        delete current;
    }
    fields.clear();

    // wipe away the layouts
    foreach (QHBoxLayout *h, overrides) delete h;
    overrides.clear();

    // clear out and set back to NULL to indicate not allocated
    if (grid1) { delete grid1; grid1 = NULL; }
    if (grid2) { delete grid2; grid2 = NULL; }
    if (vlayout1) { delete vlayout1; vlayout1 = NULL; }
    if (vlayout2) { delete vlayout2; vlayout2 = NULL; }
    if (hlayout) { delete hlayout; hlayout = NULL; }
    if (contents) { delete contents; contents = NULL; }
}

void
Form::initialise()
{
    setPalette(meta->palette);
    contents = new QWidget;
    contents->setContentsMargins(5*dpiXFactor, 20*dpiXFactor,5*dpiXFactor,5*dpiXFactor); // padding between tabbar and fields
    contents->setPalette(meta->palette);

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

void
Form::addField(FieldDefinition &x)
{
    FormField *p = new FormField(x, meta);
    fields.append(p);
    meta->addFormField(p);
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
        ((GTextEdit*)(fields[0]->widget))->setFrameStyle(QFrame::NoFrame);
        ((GTextEdit*)(fields[0]->widget))->viewport()->setAutoFillBackground(false);
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
        if (y >= rows && meta->singlecolumn==false) {
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

        if (GlobalContext::context()->specialFields.isMetric(fields[i]->definition.name)) {
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
FormField::FormField(FieldDefinition field, RideMetadata *meta) : definition(field), meta(meta), active(true)
{
    QString units;
    enabled = NULL;
    isTime = false;

    if (GlobalContext::context()->specialFields.isMetric(field.name)) {
        field.type = FIELD_DOUBLE; // whatever they say, we want a double!
        units = GlobalContext::context()->specialFields.rideMetric(field.name)->units(GlobalContext::context()->useMetricUnits);
        // remove "seconds", since the field will be a QTimeEdit field
        if (units == "seconds" || units == tr("seconds")) units = "";
        // layout units name for screen
        if (units != "") units = QString(" (%1)").arg(units);
    }

    // we need to show what units we use for weight...
    if (field.name == "Weight" && field.type == FIELD_DOUBLE) {
        units = GlobalContext::context()->useMetricUnits ? tr(" (kg)") : tr (" (lbs)");
    }

    label = new QLabel(QString("%1%2").arg(GlobalContext::context()->specialFields.displayName(field.name)).arg(units), this);
    label->setPalette(meta->palette);
    //label->setFont(font);
    //label->setFixedHeight(18);

    completer = NULL; // by default we don't have one
    switch(field.type) {

    case FIELD_TEXT : // text
    case FIELD_SHORTTEXT : // shorttext

        completer = field.getCompleter(this, meta->context->athlete->rideCache);
        widget = new QLineEdit(this);
        dynamic_cast<QLineEdit*>(widget)->setCompleter(completer);

        //widget->setFixedHeight(18);
        connect (widget, SIGNAL(textChanged(const QString&)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_TEXTBOX : // textbox
        widget = new GTextEdit(this);

        // use special style sheet ..
        dynamic_cast<GTextEdit*>(widget)->setObjectName("metadata");

        // rich text hangs 'fontd' for some users
        dynamic_cast<GTextEdit*>(widget)->setAcceptRichText(false);
        dynamic_cast<GTextEdit*>(widget)->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        if (field.name == "Change History") {
            dynamic_cast<GTextEdit*>(widget)->setReadOnly(true);
        } else {
            //we use focusout event for this now
            connect (widget, SIGNAL(focusOut(QFocusEvent*)), this, SLOT(focusOut(QFocusEvent*)));
       }
        break;

    case FIELD_INTEGER : // integer
        widget = new QSpinBox(this);
        //widget->setFixedHeight(18);
        ((QSpinBox*)widget)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        connect (widget, SIGNAL(valueChanged(int)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_DOUBLE : // double
        widget = new QDoubleSpinBox(this);
        //widget->setFixedHeight(18);
        ((QDoubleSpinBox*)widget)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        if (GlobalContext::context()->specialFields.isMetric(field.name)) {

            enabled = new QCheckBox(this);
            connect(enabled, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
            units = GlobalContext::context()->specialFields.rideMetric(field.name)->units(GlobalContext::context()->useMetricUnits);

            if (units == "seconds" || units == tr("seconds")) {
                // we need to use a TimeEdit instead
                delete widget;
                widget = new QTimeEdit(this);
                ((QTimeEdit*)widget)->setDisplayFormat("hh:mm:ss");
                connect(widget, SIGNAL(timeChanged(QTime)), this, SLOT(dataChanged()));
                isTime = true;
            } else {
                connect (widget, SIGNAL(valueChanged(double)), this, SLOT(dataChanged()));
            }
        } else {
            connect (widget, SIGNAL(valueChanged(double)), this, SLOT(dataChanged()));
        }
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_DATE : // date
        widget = new QDateEdit(this);
        ((QDateEdit*)widget)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        //widget->setFixedHeight(18);
        connect (widget, SIGNAL(dateChanged(const QDate)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_TIME : // time
        widget = new QTimeEdit(this);
        ((QTimeEdit*)widget)->setButtonSymbols(QAbstractSpinBox::NoButtons);
        //widget->setFixedHeight(18);
        ((QTimeEdit*)widget)->setDisplayFormat("hh:mm:ss");
        connect (widget, SIGNAL(timeChanged(const QTime)), this, SLOT(dataChanged()));
        connect (widget, SIGNAL(editingFinished()), this, SLOT(editFinished()));
        break;

    case FIELD_CHECKBOX : // check
        widget = new QCheckBox(this);
        //widget->setFixedHeight(18);
        connect(widget, SIGNAL(stateChanged(int)), this, SLOT(stateChanged(int)));
        break;
    }

    widget->setPalette(meta->palette);
    //widget->setFont(font);
    //connect(main, SIGNAL(rideSelected()), this, SLOT(rideSelected()));

    // if save is being called flush all the values out ready to save as they are
    connect(meta->context, SIGNAL(metadataFlush()), this, SLOT(metadataFlush()));

    active = false;
}

FormField::~FormField()
{
    delete label;

    switch (definition.type) {
        case FIELD_TEXT:
        case FIELD_SHORTTEXT:
            if (definition.name == "Keywords")
                delete (QLineEdit*)widget;
            if (completer)
                delete completer;
            break;
        case FIELD_TEXTBOX : delete ((GTextEdit*)widget); break;
        case FIELD_INTEGER : delete ((QSpinBox*)widget); break;
        case FIELD_DOUBLE : {
                                if (!isTime) delete ((QDoubleSpinBox*)widget);
                                else delete ((QTimeEdit*)widget);
                            }
                            break;
        case FIELD_DATE : delete ((QDateEdit*)widget); break;
        case FIELD_TIME : delete ((QTimeEdit*)widget); break;
        case FIELD_CHECKBOX : delete ((QCheckBox*)widget); break;
    }
    if (enabled) delete enabled;
}

void
FormField::dataChanged()
{
    if (active) return;
    edited = true;
}

void
RideMetadata::metadataFlush()
{
    // run through each field setting the metadata text to whatever
    // we have in the field
    QMapIterator<QString, Form*> d(tabList);
    d.toFront();
    while (d.hasNext()) {
       d.next();
       foreach(FormField *field, d.value()->fields) // keep track so we can destroy
          field->metadataFlush();
    }

    // now we're clean we can signal
    RideItem *item = property("ride").value<RideItem*>();
    if (item) item->notifyRideMetadataChanged();
}

void
FormField::metadataFlush()
{
    if (ourRideItem == NULL) return;

    // get values from the widgets
    QString text;

    // get the current value into a string
    switch (definition.type) {
    case FIELD_TEXT :
    case FIELD_SHORTTEXT :
             text = ((QLineEdit*)widget)->text();
             break;
    case FIELD_TEXTBOX :
        {
            text = ((GTextEdit*)widget)->document()->toPlainText();
            break;
        }

    case FIELD_INTEGER : text = QString("%1").arg(((QSpinBox*)widget)->value()); break;
    case FIELD_DOUBLE :
        {
            if (!isTime) text = QString("%1").arg(((QDoubleSpinBox*)widget)->value());
            else {
                text = QString("%1").arg(QTime(0,0,0,0).secsTo(((QTimeEdit*)widget)->time()));
            }
        }
        break;
    case FIELD_CHECKBOX :
        text = ((QCheckBox *)widget)->isChecked() ? "1" : "0";
        break;
    case FIELD_DATE : text = ((QDateEdit*)widget)->date().toString("dd.MM.yyyy"); break;
    case FIELD_TIME : text = ((QTimeEdit*)widget)->time().toString("hh:mm:ss.zzz"); break;
    }

    // Update special field
    if (definition.name == "Device") {
        ourRideItem->ride()->setDeviceType(text);

    } else if (definition.name == "Identifier") {
        ourRideItem->ride()->setId(text);

    } else if (definition.name == "Recording Interval") {
        ourRideItem->ride()->setRecIntSecs(text.toDouble());

    } else if (definition.name == "Start Date") {
        QDateTime current = ourRideItem->ride()->startTime();
        QDate date(/* year*/text.mid(6,4).toInt(),
                   /* month */text.mid(3,2).toInt(),
                   /* day */text.mid(0,2).toInt());
        QDateTime update = QDateTime(date, current.time());
        ourRideItem->setStartTime(update);

    } else if (definition.name == "Start Time") {
        QDateTime current = ourRideItem->ride()->startTime();
        QTime time(/* hours*/ text.mid(0,2).toInt(),
                   /* minutes */ text.mid(3,2).toInt(),
                   /* seconds */ text.mid(6,2).toInt(),
                   /* milliseconds */ text.mid(9,3).toInt());
        QDateTime update = QDateTime(current.date(), time);
        ourRideItem->setStartTime(update);

    } else if (definition.name != "Summary") {
        if (GlobalContext::context()->specialFields.isMetric(definition.name) && enabled->isChecked()) {

            // convert from imperial to metric if needed
            if (!GlobalContext::context()->useMetricUnits) {
                double value = text.toDouble() * (1/ GlobalContext::context()->specialFields.rideMetric(definition.name)->conversion());
                value -= GlobalContext::context()->specialFields.rideMetric(definition.name)->conversionSum();
                text = QString("%1").arg(value);
            }

            // update metric override QMap!
            QMap<QString,QString> override;
            override.insert("value", text);

            // check for compatability metrics
            QString symbol = GlobalContext::context()->specialFields.metricSymbol(definition.name);
            if (definition.name == "TSS") symbol = "coggan_tss";
            if (definition.name == "NP") symbol = "coggan_np";
            if (definition.name == "IF") symbol = "coggan_if";
            ourRideItem->ride()->metricOverrides.insert(symbol, override);

        } else {

            // we need to convert from display value to
            // stored value for the Weight field:
            if (definition.type == FIELD_DOUBLE && definition.name == "Weight" && GlobalContext::context()->useMetricUnits == false) {
                double kg = text.toDouble() / LB_PER_KG;
                text = QString("%1").arg(kg);
            }

            // just update the tags QMap!
            ourRideItem->ride()->setTag(definition.name, text);
        }
    }

}

void
FormField::focusOut(QFocusEvent *)
{
    if (ourRideItem && ourRideItem->ride()) {

        // watch to see if we actually have changed ?
        if (definition.type == FIELD_TEXTBOX && definition.name != "Change History") {

            // what did we used to be ?
            QString value = ourRideItem->ride()->getTag(definition.name, "");
            if (value != dynamic_cast<GTextEdit*>(widget)->document()->toPlainText()) {
                edited = true;
                editFinished(); // we made a change so reflect it !
            }
        }
    }
}

void
FormField::editFinished()
{
    bool changed = false;

    if (active || ourRideItem == NULL ||
        (edited == false && definition.type != FIELD_TEXTBOX) ||
        ourRideItem == NULL) return;

    // get values from the widgets
    QString text;

    // get the current value into a string
    switch (definition.type) {
    case FIELD_TEXT :
    case FIELD_SHORTTEXT :
             text = ((QLineEdit*)widget)->text();

             // we specified a value list and ignored it...
             if (text != "" && definition.values.count() && definition.values.indexOf(text) == -1) {

                 if (definition.values.count() != 1 && definition.values.at(0) != "*") // wildcard no warning
                  // just warn? - does nothing for now, in case they are just "suggestions"
                 QMessageBox::warning(this, definition.name, tr("You entered '%1' which is not an expected value.").arg(text));
             }
             break;
    case FIELD_TEXTBOX :
        {
            text = ((GTextEdit*)widget)->document()->toPlainText();
            break;
        }

    case FIELD_INTEGER : text = QString("%1").arg(((QSpinBox*)widget)->value()); break;
    case FIELD_DOUBLE : {
                            if (!isTime) text = QString("%1").arg(((QDoubleSpinBox*)widget)->value());
                            else {
                                text = QString("%1").arg(QTime(0,0,0,0).secsTo(((QTimeEdit*)widget)->time()));
                            }
                        }
                        break;
    case FIELD_DATE : text = ((QDateEdit*)widget)->date().toString("dd.MM.yyyy"); break;
    case FIELD_TIME : text = ((QTimeEdit*)widget)->time().toString("hh:mm:ss.zzz"); break;
    }

    active = true;

    // Update special field
    if (definition.name == "Device") {

        if (ourRideItem->ride()->deviceType() != text) {
            changed = true;
            ourRideItem->ride()->setDeviceType(text);
        }

    } else if (definition.name == "Identifier") {

        if (ourRideItem->ride()->id() != text) {
            changed = true;
            ourRideItem->ride()->setId(text);
        }

    } else if (definition.name == "Recording Interval") {

        if (ourRideItem->ride()->recIntSecs() != text.toDouble()) {
            changed = true;
            ourRideItem->ride()->setRecIntSecs(text.toDouble());
        }

    } else if (definition.name == "Start Date") {
        QDateTime current = ourRideItem->ride()->startTime();
        QDate date(/* year*/text.mid(6,4).toInt(),
                   /* month */text.mid(3,2).toInt(),
                   /* day */text.mid(0,2).toInt());
        QDateTime update = QDateTime(date, current.time());

        if (update != current) {
            changed = true;
            ourRideItem->setStartTime(update);

            // warn if the ride already exists with that date/time
            meta->warnDateTime(update);
        }

    } else if (definition.name == "Start Time") {
        QDateTime current = ourRideItem->ride()->startTime();
        QTime time(/* hours*/ text.mid(0,2).toInt(),
                   /* minutes */ text.mid(3,2).toInt(),
                   /* seconds */ text.mid(6,2).toInt(),
                   /* milliseconds */ text.mid(9,3).toInt());
        QDateTime update = QDateTime(current.date(), time);

        if (update != current) {

            changed = true;
            ourRideItem->setStartTime(update);

            // warn if the ride already exists with that date/time
            meta->warnDateTime(update);
        }

    } else if (definition.name != "Summary") {
        if (GlobalContext::context()->specialFields.isMetric(definition.name) && enabled->isChecked()) {

            // convert from imperial to metric if needed
            if (!GlobalContext::context()->useMetricUnits) {
                double value = text.toDouble() * (1/ GlobalContext::context()->specialFields.rideMetric(definition.name)->conversion());
                value -= GlobalContext::context()->specialFields.rideMetric(definition.name)->conversionSum();
                text = QString("%1").arg(value);
            }

            QMap<QString, QString> empty;
            QMap<QString,QString> current = ourRideItem->ride()->metricOverrides.value(GlobalContext::context()->specialFields.metricSymbol(definition.name), empty);
            QString currentvalue = current.value("value", "");

            if (currentvalue != text) {
                // update metric override QMap!
                changed = true;
                QMap<QString,QString> override;
                override.insert("value", text);
                ourRideItem->ride()->metricOverrides.insert(GlobalContext::context()->specialFields.metricSymbol(definition.name), override);
            }

        } else {

            // we need to convert from display value to
            // stored value for the Weight field:
            if (definition.type == FIELD_DOUBLE && definition.name == "Weight" && GlobalContext::context()->useMetricUnits == false) {
                double kg = text.toDouble() / LB_PER_KG;
                text = QString("%1").arg(kg);
            }

            // just update the tags QMap!
            QString current = ourRideItem->ride()->getTag(definition.name, "");
            if (current != text) {
                changed = true;
                ourRideItem->ride()->setTag(definition.name, text);
            }
        }
    }

    // default values
    if (changed) {

        // we actually edited it !
        setLinkedDefault(text);

        // and update !
        ourRideItem->notifyRideMetadataChanged();

        // rideFile is now dirty!
        ourRideItem->setDirty(true);
    }
    active = false;
}

void
FormField::setLinkedDefault(QString text)
{
    foreach (DefaultDefinition adefault, meta->getDefaults()) {
        if (adefault.field == definition.name && adefault.value == text) {
            //qDebug() << "Default for" << adefault.linkedField << ":" << adefault.linkedValue;

            if (ourRideItem->ride()->getTag(adefault.linkedField, "") == "")
                ourRideItem->ride()->setTag(adefault.linkedField, adefault.linkedValue);

            foreach (FormField *formField, meta->getFormFields()) {
                if (formField->definition.name == adefault.linkedField) {
                    formField->metadataChanged();
                    formField->setLinkedDefault(adefault.linkedValue);
                }
            }


        }

    }
}

void
RideMetadata::setLinkedDefaults(RideFile* ride)
{
    bool changed;

    do {
        changed = false;

        foreach (DefaultDefinition adefault, getDefaults())
            if (ride->getTag(adefault.field, "") == adefault.value)
                if (ride->getTag(adefault.linkedField, "") == "" && adefault.linkedValue != "") {
                    ride->setTag(adefault.linkedField, adefault.linkedValue);
                    changed = true;
                }
    } while (changed);
}

void
FormField::stateChanged(int state)
{
    if (active || ourRideItem == NULL) return; // being updated programmatically

    // are we a checkbox -- do the simple stuff
    if (definition.type == FIELD_CHECKBOX) {
        ourRideItem->ride()->setTag(definition.name, ((QCheckBox *)widget)->isChecked() ? "1" : "0");
        ourRideItem->setDirty(true);
        return;
    }

    widget->setEnabled(state ? true : false);
    widget->setHidden(state ? false : true);

    // remove from overrides if neccessary
    if (ourRideItem && ourRideItem->ride()) {

        QString symbol = GlobalContext::context()->specialFields.metricSymbol(definition.name);
        if (definition.name == "TSS") symbol = "coggan_tss";
        if (definition.name == "NP") symbol = "coggan_np";
        if (definition.name == "IF") symbol = "coggan_if";

        if (state) {
            // setup initial override value
            QMap<QString,QString> override = ourRideItem->ride()->metricOverrides.value(symbol);

            // clear and reset override value for this metric
            override.insert("value", QString("%1").arg(0.0)); // add metric value
            // update UI
            if (isTime) ((QTimeEdit*)widget)->setTime(QTime(0,0,0,0));
            else ((QDoubleSpinBox *)widget)->setValue(0.0);
            // update overrides for this metric in the main QMap
            ourRideItem->ride()->metricOverrides.insert(symbol, override);

        } else if (ourRideItem->ride()->metricOverrides.contains(symbol)) {
            // remove existing override for this metric
            ourRideItem->ride()->metricOverrides.remove(symbol);
        }

        // rideFile is now dirty!
        ourRideItem->setDirty(true);

        // get refresh done, coz overrides state has changed
        ourRideItem->notifyRideMetadataChanged();
    }
}

void
FormField::metadataChanged()
{
    if (active == true) return;

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
            if (GlobalContext::context()->specialFields.isMetric(definition.name)) {

                QString symbol = GlobalContext::context()->specialFields.metricSymbol(definition.name);
                if (definition.name == "TSS") symbol = "coggan_tss";
                if (definition.name == "NP") symbol = "coggan_np";
                if (definition.name == "IF") symbol = "coggan_if";

                //  get from metric overrides
                const QMap<QString,QString> override =
                      ourRideItem->ride()->metricOverrides.value(symbol);
                if (override.contains("value")) {
                    enabled->setChecked(true);
                    //widget->setEnabled(true);
                    widget->setHidden(false);
                    value = override.value("value");

                    // does it need conversion from metric?
                    if (GlobalContext::context()->specialFields.rideMetric(definition.name)->conversion() != 1.0) {
                        // do we want imperial?
                        if (!GlobalContext::context()->useMetricUnits) {
                            double newvalue = value.toDouble() * GlobalContext::context()->specialFields.rideMetric(definition.name)->conversion();
                            newvalue -= GlobalContext::context()->specialFields.rideMetric(definition.name)->conversionSum();
                            value = QString("%1").arg(newvalue);
                        }
                    }

                    // initialize widget to show overriden value
                    if (isTime) ((QTimeEdit*)widget)->setTime(QTime(0,0,0,0).addSecs(value.toDouble()));
                    else ((QDoubleSpinBox*)widget)->setValue(value.toDouble());
                } else {
                    value = "0.0";
                    enabled->setChecked(false);
                    //widget->setEnabled(false);
                    widget->setHidden(true);
                }
            } else {
                value = ourRideItem->ride()->getTag(definition.name, "");
            }
        }
    } else {
        value ="";
    }

    if (!enabled) { // not a ride metric 
        switch (definition.type) {
        case FIELD_TEXT : // text
        case FIELD_SHORTTEXT : // shorttext
                ((QLineEdit*)widget)->setText(value);
            break;

        case FIELD_TEXTBOX : // textbox
            if (definition.name != "Summary")
                ((GTextEdit*)widget)->setText(value);
            break;

        case FIELD_INTEGER : // integer
            ((QSpinBox*)widget)->setValue(value.toInt());
            break;

        case FIELD_DOUBLE : // double
            if (isTime) ((QTimeEdit*)widget)->setTime(QTime(0,0,0,0).addSecs(value.toDouble()));
            else {
                if (definition.name == "Weight" && GlobalContext::context()->useMetricUnits == false) {
                    double lbs = value.toDouble() * LB_PER_KG;
                    value = QString("%1").arg(lbs);
                }
                ((QDoubleSpinBox*)widget)->setValue(value.toDouble());
            }
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

        case FIELD_CHECKBOX : // checkbox
            {
            ((QCheckBox*)widget)->setChecked((value == "1") ? true : false);
            }
            break;
        }
    }
    active = false;
}

unsigned long
FieldDefinition::fingerprint(QList<FieldDefinition> list)
{
    QByteArray ba;

    foreach(FieldDefinition def, list) {

        ba.append(def.tab);
        ba.append(def.name);
        ba.append(def.type);
        ba.append(def.diary);
        ba.append(def.values.join(""));
    }

    return qChecksum(ba, ba.length());
}

QCompleter *
FieldDefinition::getCompleter(QObject *parent, RideCache* rideCache)
{
    QCompleter *completer = NULL;
    if (values.count()) {
        if (values.count() == 1 && values.at(0) == "*" && rideCache) {

            // get the metdata values from the ride cache
            QStringList past_values = rideCache->getDistinctValues(name);

            // set values from whatever we have done in the past
            completer = new QCompleter(past_values, parent);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            completer->setCompletionMode(QCompleter::InlineCompletion);

        } else {

            // user specified restricted values
            completer = new QCompleter(values, parent);
            completer->setCaseSensitivity(Qt::CaseInsensitive);
            completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
        }
    }
    return completer;
}

QString
FieldDefinition::calendarText(QString value)
{
    if (value.isEmpty() || diary != true) return QString();

    switch (type) {
        case FIELD_INTEGER:
        case FIELD_DOUBLE:
        case FIELD_DATE:
        case FIELD_TIME:
        case FIELD_CHECKBOX:
            return QString("%1: %2\n").arg(name).arg(value);
        case FIELD_TEXT:
        case FIELD_TEXTBOX:
        case FIELD_SHORTTEXT:
        default:
            return QString("%1\n").arg(value);
    }
}

unsigned long
KeywordDefinition::fingerprint(QList<KeywordDefinition> list)
{
    QByteArray ba;

    foreach(KeywordDefinition def, list) {

        ba.append(def.name);
        ba.append(def.color.name());
        ba.append(def.tokens.join(""));
    }

    return qChecksum(ba, ba.length());
}

/*----------------------------------------------------------------------
 * Read / Write metadata.xml file
 *--------------------------------------------------------------------*/

void
RideMetadata::serialize(QString filename, QList<KeywordDefinition>keywordDefinitions, QList<FieldDefinition>fieldDefinitions, QString colorfield, QList<DefaultDefinition>defaultDefinitions)
{
    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving Meta Data"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    };
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<metadata>\n";

    out << QString("<colorfield>\"%1\"</colorfield>").arg(Utils::xmlprotect(colorfield));
    //
    // First we write out the keywords
    //
    out << "\t<keywords>\n";
    // write out to file
    foreach (KeywordDefinition keyword, keywordDefinitions) {
        // chart name
        out<<QString("\t\t<keyword>\n");
        out<<QString("\t\t\t<keywordname>\"%1\"</keywordname>\n").arg(Utils::xmlprotect(keyword.name));
        out<<QString("\t\t\t<keywordcolor red=\"%1\" green=\"%3\" blue=\"%4\"></keywordcolor>\n")
                        .arg(keyword.color.red())
                        .arg(keyword.color.green())
                        .arg(keyword.color.blue());

        foreach (QString token, keyword.tokens)
            out<<QString("\t\t\t<keywordtoken>\"%1\"</keywordtoken>\n").arg(Utils::xmlprotect(token));

        out<<QString("\t\t</keyword>\n");
    }
    out <<"\t</keywords>\n";

    //
    // we write out the field definitions
    //
    out << "\t<fields>\n";
    // write out to file
    foreach (FieldDefinition field, fieldDefinitions) {
        // chart name
        out<<QString("\t\t<field>\n");
        out<<QString("\t\t\t<fieldtab>\"%1\"</fieldtab>\n").arg(Utils::xmlprotect(field.tab));
        out<<QString("\t\t\t<fieldname>\"%1\"</fieldname>\n").arg(Utils::xmlprotect(field.name));
        out<<QString("\t\t\t<fieldtype>%1</fieldtype>\n").arg(field.type);
        out<<QString("\t\t\t<fieldvalues>\"%1\"</fieldvalues>\n").arg(Utils::xmlprotect(field.values.join(",")));
        out<<QString("\t\t\t<fielddiary>%1</fielddiary>\n").arg(field.diary ? 1 : 0);
        out<<QString("\t\t</field>\n");
    }
    out <<"\t</fields>\n";

    //
    // Last we write out the default definitions
    //
    out << "\t<defaults>\n";
    // write out to file
    foreach (DefaultDefinition adefault, defaultDefinitions) {
        // chart name
        out<<QString("\t\t<default>\n");
        out<<QString("\t\t\t<defaultfield>\"%1\"</defaultfield>\n").arg(Utils::xmlprotect(adefault.field));
        out<<QString("\t\t\t<defaultvalue>\"%1\"</defaultvalue>\n").arg(Utils::xmlprotect(adefault.value));
        out<<QString("\t\t\t<defaultlinkedfield>\"%1\"</defaultlinkedfield>\n").arg(Utils::xmlprotect(adefault.linkedField));
        out<<QString("\t\t\t<defaultlinkedvalue>\"%1\"</defaultlinkedvalue>\n").arg(Utils::xmlprotect(adefault.linkedValue));
        out<<QString("\t\t</default>\n");
    }
    out <<"\t</defaults>\n";


    // end document
    out << "</metadata>\n";

    // close file
    file.close();
}

void
RideMetadata::readXML(QString filename, QList<KeywordDefinition>&keywordDefinitions, QList<FieldDefinition>&fieldDefinitions, QString &colorfield, QList<DefaultDefinition> &defaultDefinitions)
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
    colorfield = handler.getColorField();
    defaultDefinitions = handler.getDefaults();

    // backwards compatible
    if (colorfield == "") colorfield = "Calendar Text";

    // now auto append special fields, in case
    // the user wiped them or we have introduced
    // them in this release.
    bool hasCalendarText = false;
    bool hasData = false;

    for (int i=0; i<fieldDefinitions.count(); i++) {
    
        if (fieldDefinitions[i].name == "Data") {
            hasData = true;
        }
        if (fieldDefinitions[i].name == "Calendar Text") {
            hasCalendarText = true;
        }

        // other fields here...

        // any field called "Time Riding" on the "Metric" tab needs to be renamed "Time Moving" as we renamed it !
        if (fieldDefinitions[i].name == "Time Riding" && fieldDefinitions[i].tab == "Metric") fieldDefinitions[i].name = "Time Moving";

        // set type to correct value if a 1st class variable
        if (fieldDefinitions[i].name == "Device") fieldDefinitions[i].type = FIELD_SHORTTEXT;
        else if (fieldDefinitions[i].name == "Recording Interval") fieldDefinitions[i].type = FIELD_DOUBLE;
        else if (fieldDefinitions[i].name == "Start Date") fieldDefinitions[i].type = FIELD_DATE;
        else if (fieldDefinitions[i].name == "Start Time") fieldDefinitions[i].type = FIELD_TIME;
        else if (fieldDefinitions[i].name == "Identifier") fieldDefinitions[i].type = FIELD_SHORTTEXT;
    }

    if (!hasCalendarText) {
        FieldDefinition add;
        add.name = "Calendar Text";
        add.type = 1;
        add.diary = false;
        add.tab = "";

        fieldDefinitions.append(add);
    }
    if (!hasData) {
        FieldDefinition add;
        add.name = "Data";
        add.type = 2;
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
    if(qName == "keywordname") keyword.name = Utils::unprotect(buffer);
    else if(qName == "keywordtoken") keyword.tokens << Utils::unprotect(buffer);
    else if(qName == "keywordcolor") {
            // the r,g,b values are in red="xx",green="xx" and blue="xx" attributes
            // of this element and captured in startelement below
            keyword.color = QColor(red,green,blue);
    }
    else if(qName == "fieldtab") field.tab = Utils::unprotect(buffer);
    else if(qName == "fieldname") field.name =  Utils::unprotect(buffer);
    else if(qName == "fieldtype") {
        field.type = buffer.trimmed().toInt();
        if (field.tab != "" && field.type < 3 && field.name != "Filename" &&
            field.name != "Change History") field.diary = true; // default!
    } else if(qName == "fieldvalues") {
        field.values = Utils::unprotect(buffer).split(",", QString::SkipEmptyParts);
    } else if (qName == "fielddiary") field.diary = (buffer.trimmed().toInt() != 0);
    else if(qName == "defaultfield") adefault.field =  Utils::unprotect(buffer);
    else if(qName == "defaultvalue") adefault.value =  Utils::unprotect(buffer);
    else if(qName == "defaultlinkedfield") adefault.linkedField =  Utils::unprotect(buffer);
    else if(qName == "defaultlinkedvalue") adefault.linkedValue =  Utils::unprotect(buffer);

    //
    // Complex Elements
    //
    else if(qName == "keyword") // <keyword></keyword> block
        keywordDefinitions.append(keyword);
    else if (qName == "field") // <field></field> block
        fieldDefinitions.append(field);
    else if (qName == "colorfield")
        colorfield = Utils::unprotect(buffer);
    else if (qName == "default") // <default></default> block
        defaultDefinitions.append(adefault);

    return true;
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
    else if (name == "defaults")
        defaultDefinitions.clear();
    else if (name == "default")
        adefault = DefaultDefinition();

    return true;
}

bool MetadataXMLParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

bool MetadataXMLParser::endDocument()
{
    return true;
}
