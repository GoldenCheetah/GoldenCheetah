/*
 * Copyright (c) 2011 Damien Grauser
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

#include "GoldenCheetah.h"
#include "HrPwWindow.h"
#include "Context.h"
#include "LTMWindow.h"
#include "HrPwPlot.h"
#include "SmallPlot.h"
#include "RideItem.h"
#include "HelpWhatsThis.h"
#include <math.h>
#include <stdlib.h>
#include <QVector>
#include <QtGui>

// tooltip

HrPwWindow::HrPwWindow(Context *context) :
     GcChartWindow(context), context(context), current(NULL)
{
    setControls(NULL);

    //
    // reveal controls widget
    //

    // reveal controls
    rDelay = new QLabel(tr("HR Delay"));
    rDelayEdit = new QLineEdit();
    rDelayEdit->setFixedWidth(30);
    rDelaySlider = new QSlider(Qt::Horizontal);
    rDelaySlider->setTickPosition(QSlider::TicksBelow);
    rDelaySlider->setTickInterval(10);
    rDelaySlider->setMinimum(1);
    rDelaySlider->setMaximum(100);
    rDelayEdit->setValidator(new QIntValidator(rDelaySlider->minimum(),
                                                 rDelaySlider->maximum(),
                                                 rDelayEdit));
    rSmooth = new QLabel(tr("Smooth"));
    rSmoothEdit = new QLineEdit();
    rSmoothEdit->setFixedWidth(30);
    rSmoothSlider = new QSlider(Qt::Horizontal);
    rSmoothSlider->setTickPosition(QSlider::TicksBelow);
    rSmoothSlider->setTickInterval(50);
    rSmoothSlider->setMinimum(1);
    rSmoothSlider->setMaximum(500);
    rSmoothEdit->setValidator(new QIntValidator(rSmoothSlider->minimum(),
                                                rSmoothSlider->maximum(),
                                                rSmoothEdit));

    // layout reveal controls
    QHBoxLayout *r = new QHBoxLayout;
    r->setSpacing(4);
    r->setContentsMargins(0,0,0,0);
    r->addStretch();
    r->addWidget(rDelay);
    r->addWidget(rDelayEdit);
    r->addWidget(rDelaySlider);
    r->addSpacing(5);
    r->addWidget(rSmooth);
    r->addWidget(rSmoothEdit);
    r->addWidget(rSmoothSlider);
    r->addStretch();
    setRevealLayout(r);

    //
    // Chart layout
    //
	QVBoxLayout *vlayout = new QVBoxLayout;

    // main plot
    hrPwPlot = new HrPwPlot(context, this);

    HelpWhatsThis *help = new HelpWhatsThis(hrPwPlot);
    hrPwPlot->setWhatsThis(help->getWhatsThisText(HelpWhatsThis::ChartRides_HRvsPw));

    // tooltip on hover over point
    hrPwPlot->tooltip = new LTMToolTip(QwtPlot::xBottom, QwtPlot::yLeft,
                               QwtPicker::VLineRubberBand,
                               QwtPicker::AlwaysOn,
                               hrPwPlot->canvas(),
                               "");

    hrPwPlot->tooltip->setRubberBand(QwtPicker::VLineRubberBand);
    hrPwPlot->tooltip->setMousePattern(QwtEventPattern::MouseSelect1,
                                         Qt::LeftButton, Qt::ShiftModifier);
    hrPwPlot->tooltip->setTrackerPen(QColor(Qt::black));

    hrPwPlot->_canvasPicker = new LTMCanvasPicker(hrPwPlot);

    // setup the plot
    QColor inv(Qt::white);
    inv.setAlpha(0);
    hrPwPlot->tooltip->setRubberBandPen(inv);
    hrPwPlot->tooltip->setEnabled(true);
    vlayout->addWidget(hrPwPlot);
    vlayout->setStretch(0, 100);

    // just the hr and power as a plot
    smallPlot = new SmallPlot(this);
    smallPlot->setMaximumHeight(200);
    smallPlot->setMinimumHeight(100);
    vlayout->addWidget(smallPlot);
    vlayout->setStretch(1, 20);

    setChartLayout(vlayout);

    //
    // the controls
    //
    QWidget *c = new QWidget(this);
    HelpWhatsThis *helpConfig = new HelpWhatsThis(c);
    c->setWhatsThis(helpConfig->getWhatsThisText(HelpWhatsThis::ChartRides_HRvsPw));
    setControls(c);
    QFormLayout *cl = new QFormLayout(c);

    QLabel *delayLabel = new QLabel(tr("HR delay"), this);
    delayEdit = new QLineEdit(this);
    delayEdit->setFixedWidth(30);
    cl->addRow(delayLabel, delayEdit);
    delaySlider = new QSlider(Qt::Horizontal);
    delaySlider->setTickPosition(QSlider::TicksBelow);
    delaySlider->setTickInterval(1);
    delaySlider->setMinimum(1);
    delaySlider->setMaximum(100);
    delayEdit->setValidator(new QIntValidator(delaySlider->minimum(),
                                              delaySlider->maximum(),
                                              delayEdit));
    cl->addRow(delaySlider);

    smooth = 240;
    QLabel *smoothLabel = new QLabel(tr("Smooth"), this);
    smoothEdit = new QLineEdit(this);
    smoothEdit->setFixedWidth(30);
    smoothEdit->setText(QString("%1").arg(smooth));
    cl->addRow(smoothLabel, smoothEdit);
    smoothSlider = new QSlider(Qt::Horizontal);
    smoothSlider->setTickPosition(QSlider::TicksBelow);
    smoothSlider->setTickInterval(10);
    smoothSlider->setMinimum(0);
    smoothSlider->setMaximum(500);
    smoothSlider->setValue(smooth);
    smoothEdit->setValidator(new QIntValidator(smoothSlider->minimum(),
                                               smoothSlider->maximum(),
                                               smoothEdit));
    cl->addRow(smoothSlider);

    joinlineCheckBox = new QCheckBox(this);;
    joinlineCheckBox->setText(tr("Join points"));
    joinlineCheckBox->setCheckState(Qt::Unchecked);
    setJoinLineFromCheckBox();
    cl->addRow(joinlineCheckBox);

    shadeZones = new QCheckBox(this);;
    shadeZones->setText(tr("Shade Zones"));
    shadeZones->setCheckState(Qt::Checked);
    setShadeZones();
    cl->addRow(shadeZones);

    fullplot = new QCheckBox(this);;
    fullplot->setText(tr("Show Full Plot"));
    fullplot->setCheckState(Qt::Checked);
    showHidePlot();
    cl->addRow(fullplot);

    // connect them all up now initialised
    connect(hrPwPlot->_canvasPicker, SIGNAL(pointHover(QwtPlotCurve*, int)),
                            hrPwPlot, SLOT(pointHover(QwtPlotCurve*, int)));
    connect(joinlineCheckBox, SIGNAL(stateChanged(int)), this, SLOT(setJoinLineFromCheckBox()));
    connect(shadeZones, SIGNAL(stateChanged(int)), this, SLOT(setShadeZones()));
    connect(fullplot, SIGNAL(stateChanged(int)), this, SLOT(showHidePlot()));
    connect(smoothEdit, SIGNAL(editingFinished()), this, SLOT(setSmoothingFromLineEdit()));
    connect(smoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setSmoothingFromSlider()));
    connect(delayEdit, SIGNAL(editingFinished()), this, SLOT(setDelayFromLineEdit()));
    connect(delaySlider, SIGNAL(valueChanged(int)), this, SLOT(setDelayFromSlider()));
    connect(rSmoothEdit, SIGNAL(editingFinished()), this, SLOT(setrSmoothingFromLineEdit()));
    connect(rSmoothSlider, SIGNAL(valueChanged(int)), this, SLOT(setrSmoothingFromSlider()));
    connect(rDelayEdit, SIGNAL(editingFinished()), this, SLOT(setrDelayFromLineEdit()));
    connect(rDelaySlider, SIGNAL(valueChanged(int)), this, SLOT(setrDelayFromSlider()));
    connect(this, SIGNAL(rideItemChanged(RideItem*)), this, SLOT(rideSelected()));
    connect(context, SIGNAL(configChanged()), this, SLOT(configChanged()));

    // set colors etc on first run
    configChanged();
}

void
HrPwWindow::configChanged()
{
    setProperty("color", GColor(CPLOTBACKGROUND));
}

void
HrPwWindow::rideSelected()
{
    if (!amVisible())
        return;

    RideItem *ride = myRideItem;
    if (!ride || !ride->ride() || !ride->ride()->isDataPresent(RideFile::watts) || !ride->ride()->isDataPresent(RideFile::hr))
        setIsBlank(true);
    else {
        setIsBlank(false);
        setData(ride);
    }
}

void
HrPwWindow::setData(RideItem *ride)
{
    setSmooth(240);
    hrPwPlot->setDataFromRide(ride);
    smallPlot->setData(ride);
}

void
HrPwWindow::setJoinLineFromCheckBox()
{
    if (hrPwPlot->joinLine != joinlineCheckBox->isChecked()) {
        hrPwPlot->setJoinLine(joinlineCheckBox->isChecked());
        hrPwPlot->replot();
    }
}

void
HrPwWindow::setShadeZones()
{
    if (hrPwPlot->shadeZones != shadeZones->isChecked()) {
        hrPwPlot->setShadeZones(shadeZones->isChecked());
        hrPwPlot->replot();
    }
}

void
HrPwWindow::showHidePlot()
{
    if (fullplot->isChecked()) smallPlot->show();
    else smallPlot->hide();
}

void
HrPwWindow::setSmooth(int _smooth)
{
    smooth = _smooth;
    smoothSlider->setValue(_smooth);
    rSmoothSlider->setValue(_smooth);
    smoothEdit->setText(QString("%1").arg(_smooth));
    rSmoothEdit->setText(QString("%1").arg(_smooth));
    hrPwPlot->recalc();
}

void
HrPwWindow::setSmoothingFromLineEdit()
{
    int _smooth = smoothEdit->text().toInt();
    setSmooth(_smooth);
}

void
HrPwWindow::setrSmoothingFromLineEdit()
{
    int _smooth = rSmoothEdit->text().toInt();
    setSmooth(_smooth);
}

void
HrPwWindow::setSmoothingFromSlider()
{
    setSmooth(smoothSlider->value());
}

void
HrPwWindow::setrSmoothingFromSlider()
{
    setSmooth(rSmoothSlider->value());
}

void
HrPwWindow::setDelay(int delay)
{
    if (hrPwPlot->delay != delay) {
        delaySlider->setValue(delay);
        delayEdit->setText(QString("%1").arg(delay));
        rDelaySlider->setValue(delay);
        rDelayEdit->setText(QString("%1").arg(delay));
        hrPwPlot->delay = delay;
        hrPwPlot->recalc();
    }
}

void
HrPwWindow::setDelayFromLineEdit()
{
    int delay = delayEdit->text().toInt();
    setDelay(delay);
}

void
HrPwWindow::setrDelayFromLineEdit()
{
    int delay = delayEdit->text().toInt();
    setDelay(delay);
}

void
HrPwWindow::setDelayFromSlider()
{
    setDelay(delaySlider->value());
}

void
HrPwWindow::setrDelayFromSlider()
{
    setDelay(rDelaySlider->value());
}

int
HrPwWindow::findDelay(QVector<double> &wattsArray, QVector<double> &hrArray, int rideTimeSecs)
{
    int delay = 0;
    double maxr = 0;

    if (rideTimeSecs>= 60) {

        for (int a = 10; a <=60; ++a) {

            QVector<double> delayHr(rideTimeSecs);

            for (int j = a; j<rideTimeSecs; ++j) {
                delayHr[j-a] = hrArray[j];
            }
            for (int j = rideTimeSecs-a; j<rideTimeSecs; ++j) {
                delayHr[j] = 0.0;
            }

            double r = corr(wattsArray, delayHr, rideTimeSecs-a);
            //fprintf(stderr, "findDelay %d: %.2f \n", a, r);

            if (r>maxr) {
                maxr = r;
                delay = a;
            }
        }
    } 

    delayEdit->setText(QString("%1").arg(delay));
    rDelayEdit->setText(QString("%1").arg(delay));
    delaySlider->setValue(delay);
    rDelaySlider->setValue(delay);
    return delay;
}

/**************************************/
/* Fichier CPP de la librairie reglin */
/* Réalisé par GONNELLA Stéphane      */
/**************************************/


/* Déclaratin globale des variables */


/*********************/
/* Fonctions de test */
/*********************/

/* Fonction de test de présence d'un zéro */

int HrPwWindow::test_zero(QVector<double> &tab,int n)
{
        for(int i=0;i<n;i++)
        {
                if(tab[i]==0)
                { return i;}
        }

        return -1;
}

/* Fonction de test de présence d'un négatif */

int HrPwWindow::test_negatif(QVector<double> &tab,int n)
{
        for(int i=0;i<n;i++)
        {
                if(tab[i]<0)
                { return i;}
        }

        return -1;
}

/*********************************/
/* Fonctions de valeurs absolues */
/*********************************/

/*fonction qui retourne la valeur absolue*/

double HrPwWindow::val_abs(double x)
{
	if(x<0) {return -x;}
	else {return x;}
}
/********************************/
/* Fonction de recherche du max */
/********************************/

/* Fonction qui retourne celui qui est le max */

int HrPwWindow::rmax(QVector<double> &r)
{
        double temp=0;
        int ajust=0;

        for(int i=0;i<5;i++)
        {
                if(r[i]>temp)
                {
                     temp=r[i];
                     ajust = i+1;
                }
        }

        return ajust;
}

/**********************/
/* Fonctions de somme */
/**********************/

/* Fonction de somme d'éléments d'un tableau d'entier */

int HrPwWindow::somme(QVector<int> &tab,int n)
{
	int somme=0;

	for (int i=0;i<n;i++)
	{
	 somme=((tab[i])+(somme));
   	}

	return(somme);
}

/* Fonction de somme d'éléments d'un tableau de réel*/

double HrPwWindow::somme(QVector<double> &tab,int n)
{
	double somme=0;

	for (int i=0;i<n;i++)
	{
	 somme=((tab[i])+(somme));
   	}

	return(somme);
}

/**********************************/
/* Fonctions de calcul de moyenne */
/**********************************/

/* Fonction de calcul de moyenne d'éléments d'un tableau d'entier */

double HrPwWindow::moyenne(QVector<int> &tab,int n)
{
	double moyenne = double(somme(tab,n))/n;

	return (moyenne);
}

/* Fonction de calcul de moyenne d'éléments d'un tableau de réel */

double HrPwWindow::moyenne(QVector<double> &tab,int n)
{
	double moyenne = somme(tab,n)/n;

	return (moyenne);
}

/* Fonction de calcul de moyenne d'elements d'un tableau de réel(2) */

double HrPwWindow::moyenne2(double somme,int n)
{
	double moyenne = somme/n;

	return (moyenne);
}

/***********************/
/* Fonction de produit */
/***********************/

/* Fonction de calcul du produit d'éléments de deux tableau ligne par ligne */

void HrPwWindow::produittab(QVector<double> &tab1,QVector<double> &tab2,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

	for (int i=0;i<n;i++)
	{
		tab_temp[i]=(tab1[i]*tab2[i]);
	}
}

/***************************************/
/* Fonctions de changement de variable */
/***************************************/

/* Fonctions de calcul du ln des éléments d'un tableau de réel */

void HrPwWindow::lntab(QVector<double> &tab,QVector<double> &tabTemp,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

	for (int i=0;i<n;i++)
	{
		tabTemp[i]=(log(tab[i]));
	}
}

/* Fonctions de calcul du log base 10 des éléments d'un tableau de réel */

void HrPwWindow::logtab(QVector<double> &tab,QVector<double> &tabTemp,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

	for (int i=0;i<n;i++)
	{
		tabTemp[i]=(log10(tab[i]));
	}
}

/* Fonction d'inverstion des élements d'un tableau de réel */

void HrPwWindow::invtab(QVector<double> &tab,QVector<double> &tabTemp,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

	for (int i=0;i<n;i++)
	{
		tabTemp[i]=(1/tab[i]);
	}
}

/********************/
/* Autres fonctions */
/********************/

/* Fonction de calcul des écarts à la moyenne */

void HrPwWindow::ecart_a_moyenne(QVector<double> &tab,double Moyenne,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

	for (int i=0;i<n;i++)
	{
		tab_temp[i]=(tab[i] - Moyenne);
	}
}

/****************************/
/* Fonctions de statistique */
/****************************/

/* Fonction de calcul de la covariance */

double HrPwWindow::covariance(QVector<double> &Xi, QVector<double> &Yi,int n)
{
	double cov;

	produittab(Xi,Yi,n);
	cov = moyenne(tab_temp,n) - ( moyenne(Xi,n) * moyenne(Yi,n) );

	return (cov);
}

/* Fonction de calcul de la somme des carrés des écarts a la moyenne */

double HrPwWindow::variance(QVector<double> &val,int n)
{
	double sce;

	produittab(val,val,n);
	sce = moyenne(tab_temp,n) - ( moyenne(val,n) * moyenne(val,n));

  	return (sce);
}

/* Fonction de calcul de l'écart-type */

double HrPwWindow::ecarttype(QVector<double> &val,int n)
{
	double ect= sqrt(variance(val,n));

	return (ect);
}
/******************************************************/
/* Fonctions pour le calcul de la régression linéaire */
/* par la méthode des moindres carré                  */
/******************************************************/

/* Fonction de clacul de la pente (a) */

double HrPwWindow::pente(QVector<double> &Xi,QVector<double> &Yi,int n)
{
	double a = covariance(Xi,Yi,n)/variance(Xi,n);

	return (a);
}

/* Fonction de clacul de l'ordonnée a l'origine (b) */

double HrPwWindow::ordonnee(QVector<double> &Xi,QVector<double> &Yi,int n)
{
	double b = moyenne(Yi,n) - ( pente(Xi,Yi,n) * moyenne(Xi,n) );

	return (b);
}

/* Fonction de calcul du coef de corrélation (r) */

double HrPwWindow::corr(QVector<double> &Xi, QVector<double> &Yi,int n)
{
	double r = covariance(Xi,Yi,n)/(ecarttype(Xi,n)*ecarttype(Yi,n));
        //double r=pente(Xi,Yi,n)*pente(Xi,Yi,n)*(variance(Xi,n)/variance(Yi,n));
	return (r);
}

/* Fonction de détermination du meilleur ajustement */

int HrPwWindow::ajustement(QVector<double> &Xi,QVector<double> &Yi,int n)
{
        QVector<double> r(5),lnXi(100),lnYi(100),logXi(100),logYi(100),invXi(100);

        //corrélation pour linéaire

        r[0]=val_abs(corr(Xi,Yi,n));

        //corrélation pour exponetielle

        lntab(Yi,lnYi,n);
        r[1]=val_abs(corr(Xi,lnYi,n));

        //corrélation pour puissance

        logtab(Xi,logXi,n);
        logtab(Yi,logYi,n);
        r[2]=val_abs(corr(logXi,logYi,n));

        //corrélation pour inverse

        invtab(Xi,invXi,n);
        r[3]=val_abs(corr(invXi,Yi,n));

        //corrélation pour logarithmique

        lntab(Xi,lnXi,n);
        r[4]=val_abs(corr(lnXi,Yi,n));

        //Test du meilleur ajustement

        return rmax(r);
}

/*****************************/
/* Fin du fichier reglin.cpp */
/*****************************/
