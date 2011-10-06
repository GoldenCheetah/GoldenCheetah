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
};

class FormField : public QWidget
{
    Q_OBJECT

    public:
        FormField(FieldDefinition, MainWindow *, RideMetadata *meta);
        ~FormField();
        FieldDefinition definition; // define the field
        QLabel  *label;             // label
        QCheckBox *enabled;           // is the widget enabled or not?
        QWidget *widget;            // updating widget

    public slots:
        void dataChanged();         // from the widget - we changed something
        void editFinished();        // from the widget - we finished editing this field
        void rideSelected();        // from GC - a new ride got picked
        void stateChanged(int);     // should we enable/disable the widget?

    private:
        MainWindow *main;
        bool edited;                // value has been changed
        bool active;                // when data being changed for rideSelected
        SpecialFields sp;
        RideMetadata *meta;
};

class KeywordCompleter : public QCompleter
{
    Q_OBJECT

    public:
        KeywordCompleter(QWidget *parent) : QCompleter(parent) {}
        KeywordCompleter(QStringList list, QWidget *parent) : QCompleter(list, parent) {}
        QStringList splitPath(const QString &path) const;

    public slots:
        void textUpdated(const QString &);
};

class KeywordField : public QLineEdit
{
    Q_OBJECT

    public:
        KeywordField(QWidget *);

    public slots:
        void textUpdated(const QString &);
        void completeText(QString, QString);
        void completerActive(const QString &);

    private:
        KeywordCompleter *completer;
        QString full;

};

class Form : public QScrollArea
{
    Q_OBJECT

    public:
        Form(MainWindow *, RideMetadata *meta);
        ~Form();
        void addField(FieldDefinition x) { fields.append(new FormField(x, main, meta)); }
        void arrange(); // the meat of the action, arranging fields on the screen

    private:
        MainWindow *main;
        SpecialFields sp;
        QWidget *contents;
        QHBoxLayout *hlayout;
        QVBoxLayout *vlayout1, *vlayout2;
        QGridLayout *grid1, *grid2;
        QList<FormField*> fields; // keep track so we can destroy
        QList<QHBoxLayout *> overrides; // keep track so we can destroy
        RideMetadata *meta;

};

class RideMetadata : public QWidget
{
    Q_OBJECT

    public:
        RideMetadata(MainWindow *);
        static void serialize(QString filename, QList<KeywordDefinition>, QList<FieldDefinition>);
        static void readXML(QString filename, QList<KeywordDefinition>&, QList<FieldDefinition>&);
        QList<KeywordDefinition> getKeywords() { return keywordDefinitions; }
        QList<FieldDefinition> getFields() { return fieldDefinitions; }

    public slots:
        void configUpdate();
        void warnDateTime(QDateTime); // warn if file already exists after date/time changed

    private:
        MainWindow *main;
#ifdef ENABLE_METRICS_TRANSLATION
        SpecialTabs specialTabs;
#endif

    QTabWidget *tabs;
    QMap <QString, Form *> tabList;

    QStringList keywordList; // for completer
    QList<KeywordDefinition> keywordDefinitions;
    QList<FieldDefinition>   fieldDefinitions;
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

protected:
    QString buffer;

    // ths results are here
    QList<KeywordDefinition> keywordDefinitions;
    QList<FieldDefinition>   fieldDefinitions;

    // whilst parsing elements are stored here
    KeywordDefinition keyword;
    FieldDefinition   field;
    int red, green, blue;
};

#endif
