
/*
 * Metric Override Dialog Copyright (c) 2025 Paul Johnson (paulj49457@gmail.com)
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

#ifndef _MetricOverrideDialog_h
#define _MetricOverrideDialog_h

#include "GoldenCheetah.h"
#include "Context.h"

#include <QDialog>
#include <QLabel>

// Dialog class to allow overriding of metric data items
class MetricOverrideDialog : public QDialog
{
    Q_OBJECT
        G_OBJECT

    public:
        MetricOverrideDialog(Context *context, const QString& fieldName, const double value, QPoint pos);
        virtual ~MetricOverrideDialog();

    protected:

        void showEvent(QShowEvent*) override;

    private slots:

        void cancelClicked();
        void clearClicked();
        void setClicked();

    private:

        enum class DialogMetricType { DATE, SECS_TIME, MINS_TIME, DOUBLE };

        QPoint pos_;
        QString fieldName_;
        DialogMetricType dlgMetricType_ = DialogMetricType::DOUBLE;
        Context* context_ = nullptr;

        QLabel* metricLabel_ = nullptr;
        QWidget* metricEdit_ = nullptr;
};

#endif // _MetricOverrideDialog_h
