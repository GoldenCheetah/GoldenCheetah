/*
 * Copyright (c) 2011 Mark Liversedge (liversedge@gmail.com)
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

#include "NewCyclistDialog.h"
#include "Zones.h"
#include "HrZones.h"
#include "GcUpgrade.h"

NewCyclistDialog::NewCyclistDialog(QDir home) : QDialog(NULL, Qt::Dialog), home(home)
{
    setAttribute(Qt::WA_DeleteOnClose, false); // caller must delete me, once they've extracted the name
    useMetricUnits = true; // default

    QVBoxLayout *all = new QVBoxLayout(this);
    QGridLayout *grid = new QGridLayout;

    QLabel *namelabel = new QLabel(tr("Athlete Name"));
    QLabel *doblabel = new QLabel(tr("Date of Birth"));
    QLabel *sexlabel = new QLabel(tr("Sex"));
    QLabel *unitlabel = new QLabel(tr("Units"));
    QLabel *biolabel = new QLabel(tr("Bio"));
    QLabel *cplabel = new QLabel(tr("Critical Power (FTP)"));
    QLabel *wlabel = new QLabel(tr("W' (J)"));
    QLabel *wbaltaulabel = new QLabel(tr("W'bal Tau (s)"));
    QLabel *resthrlabel = new QLabel(tr("Resting Heartrate"));
    QLabel *lthrlabel = new QLabel(tr("Lactate Heartrate"));
    QLabel *maxhrlabel = new QLabel(tr("Maximum Heartrate"));

    QString weighttext = QString(tr("Weight (%1)")).arg(useMetricUnits ? tr("kg") : tr("lb"));
    weightlabel = new QLabel(weighttext);

    name = new QLineEdit(this);

    dob = new QDateEdit(this);

    sex = new QComboBox(this);
    sex->addItem(tr("Male"));
    sex->addItem(tr("Female"));

    unitCombo = new QComboBox();
    unitCombo->addItem(tr("Metric"));
    unitCombo->addItem(tr("Imperial"));

    weight = new QDoubleSpinBox(this);
    weight->setMaximum(999.9);
    weight->setMinimum(0.0);
    weight->setDecimals(1);
    weight->setValue(75); // default

    cp = new QSpinBox(this);
    cp->setMinimum(100); // juniors may average 100w for an hour, lower values might be seen for para-juniors (?)
    cp->setMaximum(500); // thats over 6w/kg for a 80kg rider, anything higher is physiologically unlikely
    cp->setSingleStep(5);      // for those that insist on using the spinners, make it a bit quicker
    cp->setValue(250);   // seems like a 'sensible' default for those that 'don't know' ?

    w = new QSpinBox(this);
    w->setMinimum(0);
    w->setMaximum(100000);
    w->setSingleStep(100);
    w->setValue(20000); // default to 20kj

    wbaltau = new QSpinBox(this);
    wbaltau->setMinimum(30);
    wbaltau->setMaximum(1200);
    wbaltau->setSingleStep(10);
    wbaltau->setValue(300); // default to 300s

    resthr = new QSpinBox(this);
    resthr->setMinimum(30);
    resthr->setMaximum(100);
    resthr->setSingleStep(1);
    resthr->setValue(60);

    lthr = new QSpinBox(this);
    lthr->setMinimum(80);
    lthr->setMaximum(220);
    lthr->setSingleStep(1);
    lthr->setValue(165);

    maxhr = new QSpinBox(this);
    maxhr->setMinimum(150);
    maxhr->setMaximum(250);
    maxhr->setSingleStep(1);
    maxhr->setValue(190);

    bio = new QTextEdit(this);

    avatar = QPixmap(":/images/noavatar.png");
    avatarButton = new QPushButton(this);
    avatarButton->setContentsMargins(0,0,0,0);
    avatarButton->setFlat(true);
    avatarButton->setIcon(avatar.scaled(140,140));
    avatarButton->setIconSize(QSize(140,140));
    avatarButton->setFixedHeight(140);
    avatarButton->setFixedWidth(140);

    Qt::Alignment alignment = Qt::AlignLeft|Qt::AlignVCenter;

    grid->addWidget(namelabel, 0, 0, alignment);
    grid->addWidget(doblabel, 1, 0, alignment);
    grid->addWidget(sexlabel, 2, 0, alignment);
    grid->addWidget(unitlabel, 3, 0, alignment);
    grid->addWidget(weightlabel, 4, 0, alignment);
    grid->addWidget(cplabel, 5, 0, alignment);
    grid->addWidget(wlabel, 6, 0, alignment);
    grid->addWidget(wbaltaulabel, 7, 0, alignment);
    grid->addWidget(resthrlabel, 8, 0, alignment);
    grid->addWidget(lthrlabel, 9, 0, alignment);
    grid->addWidget(maxhrlabel, 10, 0, alignment);
    grid->addWidget(biolabel, 11, 0, alignment);

    grid->addWidget(name, 0, 1, alignment);
    grid->addWidget(dob, 1, 1, alignment);
    grid->addWidget(sex, 2, 1, alignment);
    grid->addWidget(unitCombo, 3, 1, alignment);
    grid->addWidget(weight, 4, 1, alignment);
    grid->addWidget(cp, 5, 1, alignment);
    grid->addWidget(w, 6, 1, alignment);
    grid->addWidget(wbaltau, 7, 1, alignment);
    grid->addWidget(resthr, 8, 1, alignment);
    grid->addWidget(lthr, 9, 1, alignment);
    grid->addWidget(maxhr, 10, 1, alignment);
    grid->addWidget(bio, 12, 0, 1, 4);

    grid->addWidget(avatarButton, 0, 2, 4, 2, Qt::AlignRight|Qt::AlignVCenter);
    all->addLayout(grid);
    all->addStretch();

    // dialog buttons
    save = new QPushButton(tr("Save"), this);
    cancel = new QPushButton(tr("Cancel"), this);
    QHBoxLayout *h = new QHBoxLayout;
    h->addStretch();
    h->addWidget(cancel);
    h->addWidget(save);
    all->addLayout(h);

    connect (avatarButton, SIGNAL(clicked()), this, SLOT(chooseAvatar()));
    connect (unitCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(unitChanged(int)));
    connect (save, SIGNAL(clicked()), this, SLOT(saveClicked()));
    connect (cancel, SIGNAL(clicked()), this, SLOT(cancelClicked()));
}

void
NewCyclistDialog::chooseAvatar()
{
    QString filename = QFileDialog::getOpenFileName(this, tr("Choose Picture"),
                            "", tr("Images (*.png *.jpg *.bmp)"));
    if (filename != "") {

        avatar = QPixmap(filename);
        avatarButton->setIcon(avatar.scaled(140,140));
        avatarButton->setIconSize(QSize(140,140));
    }
}

void
NewCyclistDialog::unitChanged(int currentIndex)
{
    if (currentIndex == 0) {
        QString weighttext = QString(tr("Weight (%1)")).arg(tr("kg"));
        weightlabel->setText(weighttext);
        weight->setValue(weight->value() / LB_PER_KG);
        useMetricUnits = true;
    }
    else {
        QString weighttext = QString(tr("Weight (%1)")).arg(tr("lb"));
        weightlabel->setText(weighttext);
        weight->setValue(weight->value() * LB_PER_KG);
        useMetricUnits = false;
    }
}

void
NewCyclistDialog::cancelClicked()
{
    reject();
}

void
NewCyclistDialog::saveClicked()
{
    // if we have a name...
    if (!name->text().isEmpty()) {
        if (!home.exists(name->text())) {
            if (home.mkdir(name->text())) {

                // set the last version to the latest version
                appsettings->setCValue(name->text(), GC_VERSION_USED, GcUpgrade::version());

                // nice sidebars please!
                appsettings->setCValue(name->text(), "splitter/LTM/hide", true);
                appsettings->setCValue(name->text(), "splitter/LTM/hide/0", false);
                appsettings->setCValue(name->text(), "splitter/LTM/hide/1", false);
                appsettings->setCValue(name->text(), "splitter/LTM/hide/2", false);
                appsettings->setCValue(name->text(), "splitter/LTM/hide/3", true);
                appsettings->setCValue(name->text(), "splitter/analysis/hide", true);
                appsettings->setCValue(name->text(), "splitter/analysis/hide/0", false);
                appsettings->setCValue(name->text(), "splitter/analysis/hide/1", true);
                appsettings->setCValue(name->text(), "splitter/analysis/hide/2", false);
                appsettings->setCValue(name->text(), "splitter/analysis/hide/3", true);
                appsettings->setCValue(name->text(), "splitter/diary/hide", true);
                appsettings->setCValue(name->text(), "splitter/diary/hide/0", false);
                appsettings->setCValue(name->text(), "splitter/diary/hide/1", false);
                appsettings->setCValue(name->text(), "splitter/diary/hide/2", true);
                appsettings->setCValue(name->text(), "splitter/train/hide", true);
                appsettings->setCValue(name->text(), "splitter/train/hide/0", false);
                appsettings->setCValue(name->text(), "splitter/train/hide/1", false);
                appsettings->setCValue(name->text(), "splitter/train/hide/2", false);
                appsettings->setCValue(name->text(), "splitter/train/hide/3", false);

                // lets setup!
                if (unitCombo->currentIndex()==0)
                    appsettings->setCValue(name->text(), GC_UNIT, GC_UNIT_METRIC);
                else if (unitCombo->currentIndex()==1)
                    appsettings->setCValue(name->text(), GC_UNIT, GC_UNIT_IMPERIAL);

                appsettings->setCValue(name->text(), GC_DOB, dob->date());
                appsettings->setCValue(name->text(), GC_WEIGHT, weight->value() * (useMetricUnits ? 1.0 : KG_PER_LB));
                appsettings->setCValue(name->text(), GC_WBALTAU, wbaltau->value());
                appsettings->setCValue(name->text(), GC_SEX, sex->currentIndex());
                appsettings->setCValue(name->text(), GC_BIO, bio->toPlainText());
                avatar.save(home.path() + "/" + name->text() + "/" + "avatar.png", "PNG");

                // Setup Power Zones
                Zones zones;
                zones.addZoneRange(QDate(1900, 01, 01), cp->value(), w->value());
                zones.write(QDir(home.path() + "/" + name->text()));

                // HR Zones too!
                HrZones hrzones;
                hrzones.addHrZoneRange(QDate(1900, 01, 01), lthr->value(), resthr->value(), maxhr->value());
                hrzones.write(QDir(home.path() + "/" + name->text()));

                accept();
            } else {
                QMessageBox::critical(0, tr("Fatal Error"), tr("Can't create new directory ") + home.path() + "/" + name->text(), "OK");
            }
        } else {
            QMessageBox::critical(0, tr("Fatal Error"), tr("Athlete already exists ")  + name->text(), "OK");
        }
    } else {
        QMessageBox::critical(0, tr("Fatal Error"), tr("Athlete name is mandatory"), "OK");
    }
    return;
}
