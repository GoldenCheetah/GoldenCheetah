/*
 * Copyright (c) 2025 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#ifndef _GC_FilterSimilarDialog_h
#define _GC_FilterSimilarDialog_h 1

#include <QtGui>
#include <QTreeWidget>

#include "Context.h"
#include "RideItem.h"


class FilterSimilarDialog : public QDialog
{
    Q_OBJECT

    public:
        FilterSimilarDialog(Context *context, RideItem const * const rideItem, QWidget *parent = nullptr);

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        Context *context;
        RideItem const * const rideItem;
        QCheckBox *hideZeroed;
        QLineEdit *filterTreeEdit;
        QTreeWidget *tree;
        QTextEdit *preview;

        QString buildFilter() const;
        void addFields();
        void addMetrics();

    private slots:
        void updateFilterTree();
};

#endif
