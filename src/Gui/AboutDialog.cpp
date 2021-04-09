/*
 * Copyright (c) 2013 Mark Liversedge (liversedge@gmail.com)
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

#include "AboutDialog.h"
#include "GcUpgrade.h"
#include "GcCrashDialog.h"

AboutDialog::AboutDialog(Context *context) : context(context)
{
    setWindowTitle(tr("About GoldenCheetah"));

    aboutPage = new AboutPage(context);
    versionPage = new VersionPage(context);
    contributorsPage = new ContributorsPage(context);

    tabWidget = new QTabWidget;
    tabWidget->setContentsMargins(0,0,0,0);
    tabWidget->addTab(aboutPage, tr("About"));
    tabWidget->addTab(versionPage, tr("Version"));
    tabWidget->addTab(contributorsPage, tr("Contributors"));

    mainLayout = new QVBoxLayout;
    mainLayout->addWidget(tabWidget);

    //mainLayout->setContentsMargins(0,0,0,0);
    //mainLayout->setSpacing(0);

    setLayout(mainLayout);
}


//
// About page
//
AboutPage::AboutPage(Context *context) : context(context)
{
    QLabel *text;
    text=new QLabel(this);
    text->setContentsMargins(0,0,0,0);
    text->setOpenExternalLinks(true);
    text->setText(
              tr(
                "<center>"
                "<img src=\":images/gc.png\" height=80>"
                "<h2>GoldenCheetah</h2>"
                "Cycling Power Analysis Software<br>for Linux, Mac, and Windows"
                "<p>GoldenCheetah is licensed under the<br>"
                "<a href=\"https://opensource.org/licenses/GPL-2.0\">GNU General "
                "Public License 2.0</a>."
                "<p>Source code can be obtained from<br>"
                "<a href=\"https://www.goldencheetah.org/\">"
                "https://www.goldencheetah.org/</a>."
                "<br><p>Activity files and other data are stored in<br>"
                "<a href=\"%1\">%2</a>"
                "<p>Athlete ID %3<br>"
                "<p>Trademarks used with permission<br>"
                "BikeScore, xPower, SwimScore courtesy of <a href=\"http://www.physfarm.com\">"
                "Physfarm Training Systems</a>.<br>"
                "Virtual Speed courtesy of Tom Compton <a href=\"http://www.analyticcycling.com\">"
                "AnalyticCycling</a>.<br>"
                "<br>The core body temperature module was developed by the <br>"
                "<a href=\"http://www.usariem.army.mil/\">U.S. Army Research Institute of Environmental Medicine</a>"
                "<br> and is patent pending<br>"
                "<br><img src=\":images/services/strava_compatible.png\"/><br>"
                "</center>"
                )
                .arg(QString(QUrl::fromLocalFile(context->athlete->home->root().absolutePath()).toEncoded()))
                .arg(context->athlete->home->root().absolutePath().replace(" ", "&nbsp;"))
                .arg(context->athlete->id.toString())
    );

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(text);
    setLayout(mainLayout);
}

//
// Version page
//
VersionPage::VersionPage(Context *context) : context(context)
{
    QLabel *text;
    text=new QLabel(this);
    text->setContentsMargins(0,0,0,0);
    text->setText(GcCrashDialog::versionHTML());
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(text);

    setLayout(mainLayout);
}

//
// Contributors page
//
ContributorsPage::ContributorsPage(Context *context) : context(context)
{
    QStringList contributors;
    contributors.append("Alejandro Martinez");
    contributors.append("Alex Harsanyi");
    contributors.append("Alexandre Prokoudine");
    contributors.append("Andre Heider");
    contributors.append("Andrew Bryson");
    contributors.append("Andy Froncioni");
    contributors.append("Antonius Riha");
    contributors.append("Arun Horne");
    contributors.append("Austin Roach");
    contributors.append("Ben Walding");
    contributors.append("Berend De Schouwer");
    contributors.append("Bruno Assis");
    contributors.append("Chris Cleeland");
    contributors.append("Christian Charette");
    contributors.append("Claus Assmann");
    contributors.append("Dag Gruneau");
    contributors.append("Damien Grauser");
    contributors.append("Daniel Besse");
    contributors.append("Darren Hague");
    contributors.append("Dave Waterworth");
    contributors.append("Dean Junk");
    contributors.append("Dmitry Monakhov");
    contributors.append("Eric Brandt");
    contributors.append("Eric Christoffersen");
    contributors.append("Eric Murray");
    contributors.append("Erik Boto");
    contributors.append("Frank Zschockelt");
    contributors.append("Gareth Coco");
    contributors.append("Greg Lonnon");
    contributors.append("Henrik Johansson");
    contributors.append("Ilja Booij");
    contributors.append("Irvin Sirotić");
    contributors.append("Ivor Hewitt");
    contributors.append("Jaime Jofre");
    contributors.append("Jamie Kimberley");
    contributors.append("Jan de Visser");
    contributors.append("Jim Ley");
    contributors.append("J&#246;rn Rischm&#252;ller");
    contributors.append("Johan Martensson");
    contributors.append("John Ehrlinger");
    contributors.append("Jon Beverley");
    contributors.append("Jon Escombe");
    contributors.append("Josef Gebel");
    contributors.append("Julian Baumgartner");
    contributors.append("Julian Simioni");
    contributors.append("Justin Knotzke");
    contributors.append("Keisuke Yamaguchi");
    contributors.append("Keith Reynolds");
    contributors.append("Ken Sallot");
    contributors.append("Leif Warland");
    contributors.append("Lucas Garoto");
    contributors.append("Luke NRG");
    contributors.append("Magnus Gille");
    contributors.append("Marc Boudreau");
    contributors.append("Marcel Breij");
    contributors.append("Mark Buller");
    contributors.append("Mark Liversedge");
    contributors.append("Mark Rages");
    contributors.append("Michael Puchowicz");
    contributors.append("Michel Dagenais");
    contributors.append("Mitsukuni Sato");
    contributors.append("Ned Harding");
    contributors.append("Nicholas Feng");
    contributors.append("Nick Burrett");
    contributors.append("Okano Takayoshi");
    contributors.append("Omar Torres");
    contributors.append("Patrick McNerthney");
    contributors.append("Paul Johnson");
    contributors.append("Peter Kanatselis");
    contributors.append("Dr Phil Skiba");
    contributors.append("Philip Willoughby");
    contributors.append("Rainer Clasen");
    contributors.append("Ralf Spenneberg");
    contributors.append("Robb Romans");
    contributors.append("Robert Carlsen");
    contributors.append("Roberto Massa");
    contributors.append("Ron Alford");
    contributors.append("Satoru Kurashiki");
    contributors.append("Scott Conover");
    contributors.append("Sean Rhea");
    contributors.append("Simon Brewer");
    contributors.append("Stefan Schake");
    contributors.append("Stephan Lips");
    contributors.append("Steven Gribble");
    contributors.append("Thomas Weichmann");
    contributors.append("Tilman Schmiedeberg");
    contributors.append("Ugo Borello");
    contributors.append("Vianney Boyer");
    contributors.append("Walter B&#252;rki");
    contributors.append("Ward Muylaert");
    contributors.append("Yves Arrouye");

    QString contributorsTable = "<center><table><tr>";
    for (int i=0;i<contributors.count();i++){
        contributorsTable.append("<td><center>"+contributors.at(i)+"</center></td>");
        if ((i+1) % 4 == 0)
            contributorsTable.append("</tr><tr>");
    }
    contributorsTable.append("</tr></table></center>");

    QLabel *text;
    text=new QLabel(this);
    text->setContentsMargins(0,0,0,0);
    text->setText(contributorsTable);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0,0,0,0);
    mainLayout->addWidget(text);

    setLayout(mainLayout);
}
