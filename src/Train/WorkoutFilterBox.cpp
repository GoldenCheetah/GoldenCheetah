/*
 * Copyright (c) 2023 Joachim Kohlhammer (joachim.kohlhammer@gmx.de)
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

#include "WorkoutFilterBox.h"
#include "WorkoutFilter.h"

#include <QString>
#include <QDebug>
#include <QStyle>

#include "Colors.h"

WorkoutFilterBox::WorkoutFilterBox(QWidget *parent, Context *context) : FilterEditor(parent), context(context)
{
    QIcon workoutFilterErrorIcon = QPixmap::fromImage(QImage(":images/toolbar/popbutton.png"));
    workoutFilterErrorAction = this->addAction(workoutFilterErrorIcon, FilterEditor::LeadingPosition);
    workoutFilterErrorAction->setVisible(false);
    this->setClearButtonEnabled(true);
    this->setPlaceholderText(tr("Filter..."));
    this->setFilterCommands(workoutFilterCommands());
    connect(this, SIGNAL(textChanged(const QString&)), this, SLOT(textChanged(const QString&)));
    connect(this, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set appearance
    configChanged(CONFIG_APPEARANCE);
}

WorkoutFilterBox::~WorkoutFilterBox()
{
}

void
WorkoutFilterBox::textChanged(const QString &text)
{
    workoutFilterErrorAction->setVisible(false);
    bool ok = true;
    QString msg;
    QString input(text.trimmed());
    while (input.length() > 0 && (input.back().isSpace() || input.back() == ',')) {
        input.chop(1);
    }
    if (context && input.length() > 0) {
	filters = parseWorkoutFilter(input, ok, msg);
        if (! ok) {
            workoutFilterErrorAction->setVisible(true);
            workoutFilterErrorAction->setToolTip(tr("ERROR: %1").arg(msg));
        }
    }
}

void
WorkoutFilterBox::editingFinished()
{
    if (context && text().trimmed().length() > 0) {
        context->setWorkoutFilters(filters);
    } else {
        context->clearWorkoutFilters();
    }
}

void
WorkoutFilterBox::configChanged(qint32 topic)
{
    if (topic & CONFIG_APPEARANCE) {
        QColor color = QPalette().color(QPalette::Highlight);

        setStyleSheet(QString(
                          "QLineEdit {"
                          "    background-color: %1;"
                          "    color: %2;"
                          "    border-radius: 3px;"
                          "    border: 1px solid rgba(127,127,127,127);"
                          "}"
                          "QLineEdit:focus {"
#ifdef WIN32
                          "    border: 1px solid rgba(%3,%4,%5,255);"
#else
                          "    border: 2px solid rgba(%3,%4,%5,255);"
#endif
                          "}"
                 ).arg(GColor(CTOOLBAR).name())
                  .arg(GCColor::invertColor(GColor(CTOOLBAR)).name())
                  .arg(color.red())
                  .arg(color.green())
                  .arg(color.blue()));
    }
}
