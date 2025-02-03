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
#include "WorkoutMenuProvider.h"

#include <QString>
#include <QDebug>
#include <QStyle>

#include "Colors.h"

WorkoutFilterBox::WorkoutFilterBox(QWidget *parent, Context *context) : FilterEditor(parent), context(context)
{
    QIcon workoutFilterClearIcon = QPixmap::fromImage(QImage(":images/toolbar/clear.png"));
    QAction *workoutFilterClearAction = this->addAction(workoutFilterClearIcon, FilterEditor::TrailingPosition);
    workoutFilterClearAction->setVisible(! text().isEmpty());
    connect(this, &FilterEditor::textChanged, this, [=](const QString &text) {
        workoutFilterClearAction->setVisible(! text.isEmpty());
    });
    connect(workoutFilterClearAction, &QAction::triggered, this, [=]() {
        setText("");
    });

    QIcon workoutFilterErrorIcon = QPixmap::fromImage(QImage(":images/toolbar/popbutton.png"));
    workoutFilterErrorAction = this->addAction(workoutFilterErrorIcon, FilterEditor::LeadingPosition);
    workoutFilterErrorAction->setVisible(false);
    this->setPlaceholderText(tr("Filter..."));
    this->setFilterCommands(workoutFilterCommands());
    this->setMenuProvider(new WorkoutMenuProvider(this, QDir(gcroot).canonicalPath() + "/workoutfilters.xml"));
    connect(this, &FilterEditor::returnPressed, this, &WorkoutFilterBox::processInput);
    connect(context, SIGNAL(configChanged(qint32)), this, SLOT(configChanged(qint32)));

    // set appearance
    configChanged(CONFIG_APPEARANCE);
}

WorkoutFilterBox::~WorkoutFilterBox()
{
}


void
WorkoutFilterBox::clear
()
{
    FilterEditor::clear();
    processInput();
}


void
WorkoutFilterBox::setText
(const QString &text)
{
    FilterEditor::setText(text);
    processInput();
}


void
WorkoutFilterBox::processInput()
{
    bool errorActionWasVisible = workoutFilterErrorAction->isVisible();
    workoutFilterErrorAction->setVisible(false);
    bool ok = true;
    QString msg;
    QString input(text().trimmed());
    while (input.length() > 0 && (input.back().isSpace() || input.back() == ',')) {
        input.chop(1);
    }
    if (context) {
        if (input.length() > 0) {
            QList<ModelFilter *> filters = parseWorkoutFilter(input, ok, msg);
            if (ok) {
                context->setWorkoutFilters(filters);
            } else {
                workoutFilterErrorAction->setVisible(true);
                workoutFilterErrorAction->setToolTip(tr("ERROR: %1").arg(msg));
                context->clearWorkoutFilters();
            }
        } else {
            context->clearWorkoutFilters();
        }
    }
    if (errorActionWasVisible != workoutFilterErrorAction->isVisible()) {
        repaint();
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
