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

#include "ColorButton.h"
#include "Colors.h"

#include <QPainter>
#include <QColorDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>

ColorButton::ColorButton(QWidget *parent, QString name, QColor color, bool gc, bool ignore) : QPushButton("", parent), gc(gc), all(false), color(color), name(name)
{
#if defined(WIN32) || defined (Q_OS_LINUX)
    // are we in hidpi mode? if so undo global defaults for toolbar pushbuttons
    if (dpiXFactor > 1) {
        QString nopad = QString("QPushButton { padding-left: 0px; padding-right: 0px; "
                                "              padding-top:  0px; padding-bottom: 0px; }");
        setStyleSheet(nopad);
    }
#endif
    setColor(color);

    if (!ignore) connect(this, SIGNAL(clicked()), this, SLOT(clicked()));
};

void
ColorButton::setColor(QColor ncolor)
{
    color = ncolor;

    setFlat(true);
    QPixmap pix(20*dpiXFactor, 20*dpiYFactor);
    QPainter painter(&pix);
    if (color.isValid()) {
        painter.setPen(Qt::gray);

        // gc standard colors
        if (color.red() == 1 && color.green()==1) {
            painter.setBrush(QBrush(GColor(color.blue())));
        } else {
            painter.setBrush(QBrush(color));
        }
        painter.drawRect(0, 0, 20*dpiXFactor, 20*dpiYFactor);
    }
    QIcon icon;
    icon.addPixmap(pix);
    setIcon(icon);
    setIconSize(QSize(20*dpiXFactor, 20*dpiYFactor));
    setContentsMargins(2*dpiXFactor,2*dpiYFactor,2*dpiXFactor,2*dpiYFactor);
    setFixedSize(24 * dpiXFactor, 24 * dpiYFactor);
}

void
ColorButton::clicked()
{
    // Color picker dialog - gc uses color palettes, otherwise not
    QColor rcolor = (gc == true) ? GColorDialog::getColor(color.name(), all)
                                 : QColorDialog::getColor(color, this, tr("Choose Color"), QColorDialog::DontUseNativeDialog);

    // if we got a good color use it and notify others
    if (rcolor.isValid()) {
        setColor(rcolor);
        emit colorChosen(color);
    }
}


GColorDialog::GColorDialog(QColor selected, QWidget *parent, bool all) : QDialog(parent), original(selected), all(all)
{
    // set some flags
    setWindowTitle(tr("Choose a Color"));
    setModal(true);

    mainLayout=new QVBoxLayout(this);
    tabwidget = new QTabWidget(this);
    mainLayout->addWidget(tabwidget);

    // the standard "GC" colors
    gcdialog = new QWidget(this);
    tabwidget->addTab(gcdialog, tr("Standard"));
    QVBoxLayout *gclayout = new QVBoxLayout(gcdialog);
    QHBoxLayout *searchLayout = new QHBoxLayout();
    gclayout->addLayout(searchLayout);
    QLabel *searchLabel = new QLabel(tr("Search"), this);
    searchLayout->addWidget(searchLabel);
    searchEdit = new QLineEdit(this);
    searchLayout->addWidget(searchEdit);
    colorlist= new QTreeWidget(this);
    gclayout->addWidget(colorlist);
    QHBoxLayout *buttons = new QHBoxLayout();
    gclayout->addLayout(buttons);
    cancel = new QPushButton(tr("Cancel"));
    ok = new QPushButton(tr("OK"));
    buttons->addStretch();
    buttons->addWidget(cancel);
    buttons->addWidget(ok);

    // setup the tree widget
    colorlist->headerItem()->setText(0, tr("Color"));
    colorlist->headerItem()->setText(1, tr("Select"));
    colorlist->setColumnCount(2);
    colorlist->setColumnWidth(0,250 *dpiXFactor);
    colorlist->setSelectionMode(QAbstractItemView::SingleSelection);
    colorlist->setUniformRowHeights(true); // causes height problems when adding - in case of non-text fields
    colorlist->setIndentation(0);

    // map button signals
    mapper = new QSignalMapper(this);
    connect(mapper, SIGNAL(mappedInt(int)), this, SLOT(gcClicked(int)));

    // now add all the colours to select
    colorSet = GCColor::colorSet();
    for (int i=0; colorSet[i].name != ""; i++) {

        if (!all && colorSet[i].group != tr("Data")) continue;

        QTreeWidgetItem *add;
        ColorButton *colorButton = new ColorButton(this, colorSet[i].name, colorSet[i].color, false, true);
        // we can click the button to choose the color directly
        connect(colorButton, SIGNAL(clicked()), mapper, SLOT(map()));
        mapper->setMapping(colorButton, i);
        add = new QTreeWidgetItem(colorlist->invisibleRootItem());
        add->setData(0, Qt::UserRole, i);
        add->setText(0, colorSet[i].name);
        colorlist->setItemWidget(add, 1, colorButton);
    }

    // basic colors
    colordialog = new QColorDialog(original, this);
    colordialog->setWindowFlags(Qt::Widget);
    colordialog->setOptions(QColorDialog::DontUseNativeDialog);
    tabwidget->addTab(colordialog, tr("Custom"));

    colorlist->setSortingEnabled(true);
    colorlist->sortByColumn(0, Qt::AscendingOrder);

    // set the default to the current selection
    if (original.red() == 1 && original.green() == 1) {
        tabwidget->setCurrentIndex(0);
        for(int i=0; i<colorlist->invisibleRootItem()->childCount(); i++) {
            if (colorlist->invisibleRootItem()->child(i)->data(0, Qt::UserRole).toInt() == original.blue()) {
                colorlist->setCurrentItem(colorlist->invisibleRootItem()->child(i));
                break;
            }
        }
        colordialog->setCurrentColor(GColor(original.blue()));
    } else {
        tabwidget->setCurrentIndex(1);
        colorlist->setCurrentItem(colorlist->invisibleRootItem()->child(CPOWER));
        colordialog->setCurrentColor(original);
    }
    // returning what we got
    returning = original;

    connect(ok, SIGNAL(clicked()), this, SLOT(gcOKClicked()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
    connect(colordialog, SIGNAL(colorSelected(QColor)), this, SLOT(standardOKClicked(QColor)));
    connect(searchEdit, SIGNAL(textChanged(QString)), this, SLOT(searchFilter(QString)));

    // we ready
    show();
}

void
GColorDialog::searchFilter(QString text)
{
    QStringList toks = text.split(" ", Qt::SkipEmptyParts);
    bool empty;
    if (toks.count() == 0 || text == "") empty=true;
    else empty=false;

    for(int i=0; i<colorlist->invisibleRootItem()->childCount(); i++) {
        if (empty) colorlist->setRowHidden(i, colorlist->rootIndex(), false);
        else {
            QString text = colorlist->invisibleRootItem()->child(i)->text(0);
            bool found=false;
            foreach(QString tok, toks) {
                if (text.contains(tok, Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
            }
            colorlist->setRowHidden(i, colorlist->rootIndex(), !found);
        }
    }
}

QColor GColorDialog::getColor(QColor color, bool all)
{
    GColorDialog *dialog = new GColorDialog(color, NULL, all);
    dialog->exec();
    color = dialog->returned();
    delete dialog;
    return color;
}


void
GColorDialog::cancelClicked()
{
    returning = original;
    accept();
}

void
GColorDialog::gcClicked(int index)
{
    returning = QColor(1,1,index);
    accept();
}

void
GColorDialog::gcOKClicked()
{
    int index = colorlist->invisibleRootItem()->indexOfChild(colorlist->currentItem());
    index = colorlist->invisibleRootItem()->child(index)->data(0, Qt::UserRole).toInt();
    returning = QColor(1,1,index);
    accept();
}


void
GColorDialog::standardOKClicked(QColor color)
{
    returning = color;

    // never allow 1,1,x for general color selection
    if (returning.red()==1 && returning.green()==1) returning.setRed(0);
    accept();
}
