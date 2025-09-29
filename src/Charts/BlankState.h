/*
 * Copyright (c) 2012 Damien Grauser (Damien.Grauser@pev-geneve.ch)
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

#ifndef BLANKSTATE_H
#define BLANKSTATE_H

#include "GoldenCheetah.h"
#include "Settings.h"

#include <QtGui>
#include <QCheckBox>
#include <QLabel>

struct ShortCut
{
    QString label;
    QString text;
    QString buttonLabel;
    QString buttonIconPath;
    QMetaMethod buttonSlot;
};

class BlankStatePage : public GcWindow
{
    Q_OBJECT
    G_OBJECT

    public:
        BlankStatePage(Context *context);

        QPushButton *addToShortCuts(ShortCut shortCut);
        QCheckBox       *dontShow;

        bool canShow() { return canShow_; }
        virtual void saveState() = 0; // save settings

    signals:
        void closeClicked();

    public slots:
        void setCanShow();

    protected:
        Context *context;
        bool canShow_;

        QVBoxLayout     *leftLayout;
        QLabel          *welcomeTitle;
        QLabel          *welcomeText;

        QToolButton     *img;

        QPushButton     *closeButton;


    private:
        QList<ShortCut> shortCuts;
};

class BlankStateAnalysisPage : public BlankStatePage
{
    Q_OBJECT
    G_OBJECT

    public:
        BlankStateAnalysisPage(Context *context);
        void saveState();

};

class BlankStateHomePage : public BlankStatePage
{
    Q_OBJECT
    G_OBJECT

    public:
        BlankStateHomePage(Context *context);
        void saveState();

};

class BlankStatePlanPage : public BlankStatePage
{
    Q_OBJECT
    G_OBJECT

    public:
        BlankStatePlanPage(Context *context);
        void saveState();

};

class BlankStateTrainPage : public BlankStatePage
{
    Q_OBJECT
    G_OBJECT

    public:
        BlankStateTrainPage(Context *context);
        void saveState();

};

#endif
