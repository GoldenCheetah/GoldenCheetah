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

#include "LTMChartParser.h"
#include "LTMSettings.h"
#include "LTMTool.h"
#include "Athlete.h"
#include "Utils.h"

#include <QDate>
#include <QDebug>

bool LTMChartParser::startDocument()
{
    buffer.clear();
    return true;
}

// to see the format of the charts.xml file, look at the serialize()
// function at the bottom of this source file.
bool LTMChartParser::endElement( const QString&, const QString&, const QString &qName )
{
    //
    // Single Attribute elements
    //
    if(qName == "chart") {
        QString string = buffer.mid(1,buffer.length()-2);

        QByteArray base64(string.toLatin1());
        QByteArray unmarshall = QByteArray::fromBase64(base64);
        QDataStream s(&unmarshall, QIODevice::ReadOnly);
        LTMSettings x;
        s >> x;
        settings << x;
    } 
    return true;
}

bool LTMChartParser::startElement( const QString&, const QString&, const QString &, const QXmlAttributes &)
{
    buffer.clear();
    return true;
}

bool LTMChartParser::characters( const QString& str )
{
    buffer += str;
    return true;
}

QList<LTMSettings>
LTMChartParser::getSettings()
{
    return settings;
}

bool LTMChartParser::endDocument()
{
    return true;
}

//
// Write out the charts.xml file
//
// Most of the heavy lifting is done by the LTMSettings
// << and >> operators. We just put them into the character
// data for a chart.
//
void
LTMChartParser::serialize(QString filename, QList<LTMSettings> charts)
{
    // open file - truncate contents
    QFile file(filename);
    if (!file.open(QFile::WriteOnly)) {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setText(tr("Problem Saving Charts Configuration"));
        msgBox.setInformativeText(tr("File: %1 cannot be opened for 'Writing'. Please check file properties.").arg(filename));
        msgBox.exec();
        return;
    };
    file.resize(0);
    QTextStream out(&file);
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif

    serializeToQTextStream(out, charts);

    // close file
    file.close();
}

void
LTMChartParser::serializeToQString(QString* string, QList<LTMSettings> charts)
{
    QTextStream out(string);
#if QT_VERSION < 0x060000
    out.setCodec("UTF-8");
#endif

    serializeToQTextStream(out, charts);

    // write all to out
    out.flush();

}

void
LTMChartParser::serializeToQTextStream(QTextStream& out, QList<LTMSettings>& charts)
{
    // begin document
    out << QString("<charts version=\"%1\">\n").arg(LTM_VERSION_NUMBER);

    // write out to file
    foreach (LTMSettings chart, charts) {

        out <<"\t<chart name=\"" << Utils::xmlprotect(chart.name) <<"\">\"";
        // chart name
        QByteArray marshall;
        QDataStream s(&marshall, QIODevice::WriteOnly);
        s << chart;
        out<<marshall.toBase64();
        out<<"\"</chart>\n";
    }

    // end document
    out << "</charts>\n";

}


ChartTreeView::ChartTreeView(Context *context) : context(context)
{
    setDragDropMode(QAbstractItemView::InternalMove);
    setDragDropOverwriteMode(true);
#ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
#endif

}

void
ChartTreeView::dropEvent(QDropEvent* event)
{
    QTreeWidgetItem* target = (QTreeWidgetItem *)itemAt(event->position().toPoint());
    int idxTo = indexFromItem(target).row();

    // when dragging to past the last one, we get -1, so lets
    // set the target row to the very last one
    if (idxTo < 0) idxTo = invisibleRootItem()->childCount()-1;

    int offsetFrom = 0;
    int offsetTo = 0;

    QList<int> idxToList;

    foreach (QTreeWidgetItem *p, selectedItems()) {
        int idxFrom = indexFromItem(p).row();

        context->athlete->presets.move(idxFrom+offsetFrom, idxTo+offsetTo);

        idxToList << idxTo+(idxFrom>idxTo?offsetTo:offsetFrom);

        if (idxFrom<idxTo)
            offsetFrom--;
        else
            offsetTo++;
    }

    // the default implementation takes care of the actual move inside the tree
    //QTreeWidget::dropEvent(event);

    // reset
    context->notifyPresetsChanged();

    clearSelection();
    // xxx dgr removed because
    // select it!
    /*foreach (int idx, idxToList) {
        invisibleRootItem()->child(idx)->setSelected(true);
    }*/
}

QStringList
ChartTreeView::mimeTypes() const
{
    QStringList returning;
    returning << "application/x-gc-charts";

    return returning;
}

QMimeData *
ChartTreeView::mimeData
#if QT_VERSION < 0x060000
(const QList<QTreeWidgetItem *> items) const
#else
(const QList<QTreeWidgetItem *> &items) const
#endif
{
    QMimeData *returning = new QMimeData;

    // we need to pack into a byte array
    QByteArray rawData;
    QDataStream stream(&rawData, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_4_6);

    // pack data
    stream << (quint64)(context); // where did this come from?
    stream << items.count();
    foreach (QTreeWidgetItem *p, items) {

        // get the season this is for
        int index = p->treeWidget()->invisibleRootItem()->indexOfChild(p);

        // season[index] ...
        stream << context->athlete->presets[index]; // name
    }

    // and return as mime data
    returning->setData("application/x-gc-charts", rawData);
    return returning;
}
