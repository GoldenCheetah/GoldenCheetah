/*
 * Copyright (c) 2014 Joern Rischmueller (joern.rm@gmail.com)
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

#ifndef _GC_RideAutoImportConfig_h
#define _GC_RideAutoImportConfig_h

#include "GoldenCheetah.h"
#include <QXmlDefaultHandler>
#include <QString>


class RideAutoImportRule {

    Q_DECLARE_TR_FUNCTIONS(RideAutoImportRule)

public:
    static QList<QString> rules;
    enum ImportRule { noImport=0, ignoreDuplicateErrors=1, ignoreAllErrors=2 };

    RideAutoImportRule();

    void setDirectory(QString);
    QString getDirectory();

    void setImportRule(int);
    int getImportRule();

    QList<QString> getRuleDescriptions();

private:
    QString _directory;
    int _importRule; // enum
    QList<QString> _ruleDescriptions;


};


class RideAutoImportConfig : public QObject {

    Q_OBJECT

    public:
        RideAutoImportConfig(QDir config) : config(config) { readConfig(); }

        void readConfig();
        void writeConfig();
        QList<RideAutoImportRule> getConfig() { return _configList; }
        bool hasRules() { return (_configList.count() > 0); }

    signals:
        void changedConfig();

    private:
        QDir config;
        QList<RideAutoImportRule> _configList;
};


class RideAutoImportConfigParser : public QXmlDefaultHandler
{

public:
    // marshall
    static bool serialize(QString, QList<RideAutoImportRule> rules);

    // unmarshall
    bool startDocument();
    bool endDocument();
    bool endElement( const QString&, const QString&, const QString &qName );
    bool startElement(const QString&, const QString&, const QString &name, const QXmlAttributes& );
    bool characters( const QString& str );
    QList<RideAutoImportRule> getRules();

private:
    QString buffer;
    RideAutoImportRule rule;
    QList<RideAutoImportRule> rules;
    static QString EncodeXML ( const QString& );
    static QString DecodeXML ( const QString& );

};


#endif
