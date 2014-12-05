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

#include "RideAutoImportConfig.h"
#include "Context.h"
#include "Athlete.h"


// Class: RideAutoImportRule


RideAutoImportRule::RideAutoImportRule() {
    _directory = "";
    _importRule = noImport;

    _ruleDescriptions.append(tr("No autoimport"));
    _ruleDescriptions.append(tr("Autoimport with dialog"));

}

void
RideAutoImportRule::setDirectory(QString dir) { _directory = dir; }

QString
RideAutoImportRule::getDirectory() { return _directory; }

void
RideAutoImportRule::setImportRule(int rule) { _importRule = rule; }

int
RideAutoImportRule::getImportRule() { return _importRule; }

QList<QString>
RideAutoImportRule::getRuleDescriptions() { return _ruleDescriptions; }


// Class: RideAutoImportConfig

void
RideAutoImportConfig::readConfig()
{
    QFile autoImportRulesFile(config.canonicalPath() + "/autoimport.xml");
    QXmlInputSource source( &autoImportRulesFile );
    QXmlSimpleReader xmlReader;
    RideAutoImportConfigParser handler;
    xmlReader.setContentHandler(&handler);
    xmlReader.setErrorHandler(&handler);
    xmlReader.parse(source);

    // go read them!
    _configList = handler.getRules();

    // let everyone know they have changed
    changedConfig();
}

void
RideAutoImportConfig::writeConfig()
{
    // update seasons.xml
    QString file = QString(config.canonicalPath() + "/autoimportrules.xml");
    RideAutoImportConfigParser::serialize(file, _configList);

    changedConfig(); // signal!
}

// Class: RideAutoImportConfigParser

bool
RideAutoImportConfigParser::startDocument()
{
    buffer.clear();
    return true;
}

bool
RideAutoImportConfigParser::endElement( const QString&, const QString&, const QString &qName )
{
    if(qName == "directory") {
        rule.setDirectory(DecodeXML(buffer.trimmed()));
        buffer.clear();
    }
    else if(qName == "importrule") {
        rule.setImportRule(buffer.trimmed().toInt());
        buffer.clear();
    }
     else if(qName == "rule") {
        rules.append(rule);
        buffer.clear();
    }
    return true;
}

bool
RideAutoImportConfigParser::startElement( const QString&, const QString&, const QString &name, const QXmlAttributes& )
{
    buffer.clear();
    if(name == "rule") {
        rule = RideAutoImportRule();
    }

    return true;
}

bool
RideAutoImportConfigParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

QList<RideAutoImportRule>
RideAutoImportConfigParser::getRules()
{
    return rules;
}

bool
RideAutoImportConfigParser::endDocument()
{
    return true;
}

bool
RideAutoImportConfigParser::serialize(QString filename, QList<RideAutoImportRule> rules)
{
    // open file - truncate contents
    QFile file(filename);
    file.open(QFile::WriteOnly);
    file.resize(0);
    QTextStream out(&file);
    out.setCodec("UTF-8");

    // begin document
    out << "<autoimportrules>\n";

    // write out to file
    foreach (RideAutoImportRule rule, rules) {

            QString dir = rule.getDirectory();

            out<<QString("\t<rule>\n"
                  "\t\t<directory>%1</directory>\n"
                  "\t\t<importrule>%2</importrule>\n").arg(EncodeXML(dir))
                                                      .arg(QString::number(rule.getImportRule()));
            out <<QString("\t</rule>\n");
    }

    // end document
    out << "</autoimportrules>\n";

    // close file
    file.close();

    return true; // success
}

QString RideAutoImportConfigParser::EncodeXML ( const QString& encodeMe )
{
    QString temp;

    for (int index(0); index < encodeMe.size(); index++)
    {
        QChar character(encodeMe.at(index));

        switch (character.unicode())
        {
        case '&':
            temp += "&amp;"; break;

        case '\'':
            temp += "&apos;"; break;

        case '"':
            temp += "&quot;"; break;

        case '<':
            temp += "&lt;"; break;

        case '>':
            temp += "&gt;"; break;

        default:
            temp += character;
            break;
        }
    }

    return temp;
}

QString RideAutoImportConfigParser::DecodeXML ( const QString& decodeMe )
{
    QString temp(decodeMe);

    temp.replace("&amp;", "&");
    temp.replace("&apos;", "'");
    temp.replace("&quot;", "\"");
    temp.replace("&lt;", "<");
    temp.replace("&gt;", ">");

    return temp;
}



