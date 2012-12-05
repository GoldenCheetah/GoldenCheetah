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

#include "MainWindow.h"
#include "SpecialFields.h"
#include <QWidget>
#include <QXmlDefaultHandler>

// field types
#define FIELD_TEXT      0
#define FIELD_TEXTBOX   1
#define FIELD_SHORTTEXT 2
#define FIELD_INTEGER   3
#define FIELD_DOUBLE    4
#define FIELD_DATE      5
#define FIELD_TIME      6
#define FIELD_CHECKBOX  7

class KeywordDefinition
{
    public:
        QString name;       // keyword for autocomplete
        QColor  color;      // color to highlight with
        QStringList tokens; // texts to find from notes
};

class FieldDefinition
{
    public:
        QString tab,
                name;
        int type;
        bool diary; // show in summary on diary page...
        QStringList values; // autocomplete 'defaults'

    FieldDefinition() : tab(""), name(""), type(0), diary(false), values() {}
    FieldDefinition(QString tab, QString name, int type, bool diary, QStringList values)
                      : tab(tab), name(name), type(type), diary(diary), values(values) {}
};

class FormField : public QWidget
{
    Q_OBJECT
    G_OBJECT


    public:
        FormField(FieldDefinition, RideMetadata *);
        ~FormField();
        FieldDefinition definition; // define the field
        QLabel  *label;             // label
        QCheckBox *enabled;           // is the widget enabled or not?
        QWidget *widget;            // updating widget
        QCompleter *completer;      // for completion

    public slots:
        void dataChanged();         // from the widget - we changed something
        void editFinished();        // from the widget - we finished editing this field
        void metadataChanged();     // from GC - a new ride got picked / changed elsewhere
        void stateChanged(int);     // should we enable/disable the widget?

    private:
        RideMetadata *meta;
        bool edited;                // value has been changed
        bool active;                // when data being changed for rideSelected
};

class Form : public QScrollArea
{
    Q_OBJECT
    G_OBJECT


    public:
        Form(RideMetadata *);
        ~Form();
        void addField(FieldDefinition &x);
        void arrange(); // the meat of the action, arranging fields on the screen
        void clear();  // destroy contents prior to delete
        void initialise();  // re-initialise contents (after clear)

        QVector<FormField*> fields; // keep track so we can destroy
        QVector<QHBoxLayout *> overrides; // keep track so we can destroy
    private:
        RideMetadata *meta;
        QWidget *contents;
        QHBoxLayout *hlayout;
        QVBoxLayout *vlayout1, *vlayout2;
        QGridLayout *grid1, *grid2;

};

class RideMetadata : public QWidget
{
    Q_OBJECT
    G_OBJECT
    Q_PROPERTY(RideItem *ride READ rideItem WRITE setRideItem)
    RideItem *_ride, *_connected;

    public:
        RideMetadata(MainWindow *, bool singlecolumn = false);
        static void serialize(QString filename, QList<KeywordDefinition>, QList<FieldDefinition>, QString colofield);
        static void readXML(QString filename, QList<KeywordDefinition>&, QList<FieldDefinition>&, QString &colorfield);
        QList<KeywordDefinition> getKeywords() { return keywordDefinitions; }
        QList<FieldDefinition> getFields() { return fieldDefinitions; }

        QString getColorField() const { return colorfield; }
        void setColorField(QString x) { colorfield = x; }

        void setRideItem(RideItem *x);
        RideItem *rideItem() const;

        bool singlecolumn;
        SpecialFields sp;

        MainWindow *main;
        SpecialTabs specialTabs;

    public slots:
        void configUpdate();
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

    QString colorfield;
};

class MetadataXMLParser : public QXmlDefaultHandler
{

public:
    bool startDocument() { return TRUE; }
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement( const QString&, const QString&, const QString &name, const QXmlAttributes &attrs );
    bool characters( const QString& str );

    QList<KeywordDefinition> getKeywords() { return keywordDefinitions; }
    QList<FieldDefinition> getFields() { return fieldDefinitions; }
    QString getColorField() { return colorfield; }

protected:
    QString buffer;

    // ths results are here
    QList<KeywordDefinition> keywordDefinitions;
    QList<FieldDefinition>   fieldDefinitions;
    QString colorfield;

    // whilst parsing elements are stored here
    KeywordDefinition keyword;
    FieldDefinition   field;
    int red, green, blue;
};

#endif
