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

#ifndef _GC_RideMetadata_h
#define _GC_RideMetadata_h
#include "GoldenCheetah.h"

#include "Context.h"
#include "SpecialFields.h"

#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QXmlDefaultHandler>
#include <QTextEdit>
#include <QCompleter>
#include <QMessageBox>

// field types
#define FIELD_TEXT      0
#define FIELD_TEXTBOX   1
#define FIELD_SHORTTEXT 2
#define FIELD_INTEGER   3
#define FIELD_DOUBLE    4
#define FIELD_DATE      5
#define FIELD_TIME      6
#define FIELD_CHECKBOX  7

class RideMetadata;
class RideFileInterval;
class RideEditor;

class KeywordDefinition
{
    public:

        QString filterExpression;   // textual version of the keyword filter
        QColor color;               // color to highlight if item matches keyword filter

        static unsigned long fingerprint(QList<KeywordDefinition>);
};

class FieldDefinition
{
    public:

        QString tab,
                name;
        int type;
        bool diary; // show in summary on diary page...
        bool interval; // this is interval specific metadata

        QStringList values; // autocomplete 'defaults'
        QString expression; // expression to evaluate, if true field is available

        static unsigned long fingerprint(QList<FieldDefinition>);
        QCompleter *getCompleter(QObject *parent, RideCache *rideCache);
        QString calendarText(QString value);

        FieldDefinition() : tab(""), name(""), type(0), diary(false), interval(false), values(), expression("") {}
        FieldDefinition(QString tab, QString name, int type, bool diary, bool interval, QStringList values, QString expression)
                        : tab(tab), name(name), type(type), diary(diary), interval(interval), values(values), expression(expression) {}
};

class Form;
class FormField : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        FormField(Form *, FieldDefinition, RideMetadata *);
        ~FormField();
        FieldDefinition definition; // define the field
        Form *form;                 // the form we are on
        QLabel  *label;             // label
        QCheckBox *enabled;           // is the widget enabled or not?
        QWidget *widget;            // updating widget
        QCompleter *completer;      // for completion

        void setLinkedDefault(QString text);    // set linked fields with default value

    public slots:
        void dataChanged();         // from the widget - we changed something
        void editFinished();        // from the widget - we finished editing this field
        void metadataChanged();     // from GC - a new ride got picked / changed elsewhere
        void stateChanged(int);     // should we enable/disable the widget?
        void focusOut(QFocusEvent *event=NULL);
        void metadataFlush();       // update all the tags to whatever is edited now

    private:
        RideMetadata *meta;
        bool edited;                // value has been changed
        bool active;                // when data being changed for rideSelected
        bool isTime;                // when we edit metrics but they are really times
        bool warn;                  // a warning is displayed
};

class Form : public QScrollArea
{
    Q_OBJECT
    G_OBJECT


    public:
        Form(RideMetadata *);
        ~Form();

        // ride item or intervals are selected
        void rideSelected(RideItem *ride);
        void intervalSelected(IntervalItem *interval);
        void metadataChanged();

        void addField(FieldDefinition &x);
        void arrange(); // the meat of the action, arranging fields on the screen
        void clear();  // destroy contents prior to delete
        void initialise();  // re-initialise contents (after clear)

        QVector<FormField*> fields; // keep track so we can destroy
        QVector<QHBoxLayout *> overrides; // keep track so we can destroy

        // interval navigation
        bool hasintervals;
        RideFileInterval *interval;
        QPushButton *left, *right;
        QLabel *intervalname;

    public slots:
        // user switching interval using the selector
        void intervalLeft();
        void intervalRight();

    private:
        RideMetadata *meta;
        QWidget *contents;
        QHBoxLayout *hlayout;
        QVBoxLayout *vlayout1, *vlayout2;
        QGridLayout *grid1, *grid2;

};

class DefaultDefinition
{
    public:
        QString field;
        QString value;
        QString linkedField;
        QString linkedValue;


    DefaultDefinition() : field(""), value(""), linkedField(""), linkedValue("") {}
    DefaultDefinition(QString field, QString value, QString linkedField, QString linkedValue)
                      : field(field), value(value), linkedField(linkedField), linkedValue(linkedValue) {}
};

class RideMetadata : public QWidget
{
    Q_OBJECT
    G_OBJECT
    Q_PROPERTY(RideItem *ride READ rideItem WRITE setRideItem)
    RideItem *_ride, *_connected;

    public:
        RideMetadata(Context *, bool singlecolumn = false);
        static void serialize(QString filename, QList<KeywordDefinition>, QList<FieldDefinition>, QList<DefaultDefinition>defaultDefinitions);
        static void readXML(QString filename, QList<KeywordDefinition>&, QList<FieldDefinition>&, QList<DefaultDefinition>&defaultDefinitions);
        QList<KeywordDefinition> getKeywords() { return keywordDefinitions; }
        QList<FieldDefinition> getFields() { return fieldDefinitions; }
        QList<DefaultDefinition> getDefaults() { return defaultDefinitions; }
        bool hasCalendarText();
        QString calendarText(RideItem *rideItem);

        QStringList sports();

        QColor getColor(RideItem* rideItem) const;
        QColor getReverseColor() const;

        void setRideItem(RideItem *x);
        RideItem *rideItem() const;

        void addFormField(FormField *f);
        QVector<FormField*> getFormFields();

        bool singlecolumn;

        Context *context;
        SpecialTabs specialTabs;

        QPalette palette; // to be applied to all widgets

        void setLinkedDefaults(RideFile* ride);

        bool active;            // ignore signals when editing is active

    public slots:
        void configChanged(qint32);
        void intervalSelected();
        void intervalsChanged(); // when intervals are edited or deleted

        void metadataFlush();
        void metadataChanged(); // when its changed elsewhere we need to refresh fields
        void setExtraTab();     // shows fields not configured but present in ride file
        void warnDateTime(QDateTime); // warn if file already exists after date/time changed

    private:


    QTabWidget *tabs;
    QMap <QString, Form *> tabList;
    Form *extraForm;
    QList<FieldDefinition> extraDefs;

    QStringList keywordList; // for completer
    QList<KeywordDefinition> keywordDefinitions;
    QList<FieldDefinition>   fieldDefinitions;
    QList<DefaultDefinition>   defaultDefinitions;

    QVector<FormField*>   formFields;

    QString colorfield; // support legacy format conversion
    QColor reverseColor;

    RideEditor *editor;
};

class MetadataXMLParser : public QXmlDefaultHandler
{

public:
    bool startDocument() { return true; }
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

    QList<KeywordDefinition> getKeywords() { return keywordDefinitions; }
    QList<FieldDefinition> getFields() { return fieldDefinitions; }
    QList<DefaultDefinition> getDefaults() { return defaultDefinitions; }

protected:
    QString buffer;

    // ths results are here
    QList<KeywordDefinition> keywordDefinitions;
    QList<FieldDefinition>   fieldDefinitions;
    QString colorfield; // legacy field for conversion
    QList<DefaultDefinition>   defaultDefinitions;

    // whilst parsing elements are stored here
    KeywordDefinition keyword;
    FieldDefinition   field;
    DefaultDefinition   adefault;
    int red, green, blue;
};

// version of qtextedit with signal when lost focus
class GTextEdit : public QTextEdit
{
    Q_OBJECT

    public:
        GTextEdit(QWidget*parent) : QTextEdit(parent) {}
        void focusOutEvent (QFocusEvent *event) { emit focusOut(event); }

    signals:
        void focusOut(QFocusEvent *);
};
#endif
