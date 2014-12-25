/*
 * Copyright (c) 2014 Damien Grauser
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

#include <cmath>
#include <float.h>
#include "Statistic.h"

#include <QDebug>

Statistic::Statistic() :
          minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
          points(0.0), sumX(0.0), sumY(0.0), sumXsquared(0.0),
          sumYsquared(0.0), sumXY(0.0), a(0.0), b(0.0), c(1.0)
{
    tab_temp.resize(1);
}

Statistic::Statistic(double *xdata, double *ydata, int count) :
          minX(0.0), maxX(0.0), minY(0.0), maxY(0.0),
          points(0.0), sumX(0.0), sumY(0.0), sumXsquared(0.0),
          sumYsquared(0.0), sumXY(0.0), a(0.0), b(0.0), c(1.0)
{
    if (count <= 2) return;

    for (int i = 0; i < count; i++) {

        // ignore zero points
        //if (ydata[i] == 0.00) continue;

        points++;
        sumX += xdata[i];
        sumY += ydata[i];
        sumXsquared += xdata[i] * xdata[i];
        sumYsquared += ydata[i] * ydata[i];
        sumXY += xdata[i] * ydata[i];

        if (xdata[i]>maxX)
            maxX = xdata[i];
        else if (xdata[i]<minX)
            minX = xdata[i];

        if (ydata[i]>maxY)
            maxY = ydata[i];
        else if (ydata[i]<minY)
            minY = ydata[i];
    }

    if (fabs( double(points) * sumXsquared - sumX * sumX) > DBL_EPSILON) {
        b = ( double(points) * sumXY - sumY * sumX) /
            ( double(points) * sumXsquared - sumX * sumX);
        a = (sumY - b * sumX) / double(points);
        c = ( double(points) * sumXY - sumY * sumX) /
            sqrt(( double(points) * sumXsquared - sumX * sumX) * ( double(points) * sumYsquared - sumY * sumY));


        double sx = b * ( sumXY - sumX * sumY / double(points) );
        double sy2 = sumYsquared - sumY * sumY / double(points);
        double sy = sy2 - sx;

        double coefD = sx / sy2;
        double coefC = sqrt(coefD);
        double stdError = sqrt(sy / double(points - 2));

        qDebug() << "Statistic" << coefD << coefC << stdError << c;
    }
}

QString
Statistic::label()
{
    return QString("%1x%2%3 (%4)").arg(QString("").setNum(slope(), 'f', 3)).arg(intercept()>=0?"+":"").arg(QString("").setNum(intercept(), 'f', 1)).arg(QString("").setNum(r(), 'f', 3));
}

/**************************************/
/* librairie reglin                   */
/* Réalisé par GONNELLA Stéphane      */
/**************************************/


/* Déclaratin globale des variables */


/*********************/
/* Fonctions de test */
/*********************/

/* Fonction de test de présence d'un zéro */

int
Statistic::test_zero(QVector<double> &tab,int n)
{
        for(int i=0;i<n;i++)
        {
                if(tab[i]==0)
                { return i;}
        }

        return -1;
}

/* Fonction de test de présence d'un négatif */

int
Statistic::test_negatif(QVector<double> &tab,int n)
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

double
Statistic::val_abs(double x)
{
    if(x<0) {return -x;}
    else {return x;}
}
/********************************/
/* Fonction de recherche du max */
/********************************/

/* Fonction qui retourne celui qui est le max */

int
Statistic::rmax(QVector<double> &r)
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

int
Statistic::somme(QVector<int> &tab,int n)
{
    int somme=0;

    for (int i=0;i<n;i++)
    {
     somme=((tab[i])+(somme));
    }

    return(somme);
}

/* Fonction de somme d'éléments d'un tableau de réel*/

double
Statistic::somme(QVector<double> &tab,int n)
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

double
Statistic::moyenne(QVector<int> &tab,int n)
{
    double moyenne = double(somme(tab,n))/n;

    return (moyenne);
}

/* Fonction de calcul de moyenne d'éléments d'un tableau de réel */

double
Statistic::moyenne(QVector<double> &tab,int n)
{
    double moyenne = somme(tab,n)/n;

    return (moyenne);
}

/* Fonction de calcul de moyenne d'elements d'un tableau de réel(2) */

double
Statistic::moyenne2(double somme,int n)
{
    double moyenne = somme/n;

    return (moyenne);
}

/***********************/
/* Fonction de produit */
/***********************/

/* Fonction de calcul du produit d'éléments de deux tableau ligne par ligne */

void
Statistic::produittab(QVector<double> &tab1,QVector<double> &tab2,int n)
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

void
Statistic::lntab(QVector<double> &tab,QVector<double> &tabTemp,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        tabTemp[i]=(log(tab[i]));
    }
}

/* Fonctions de calcul du log base 10 des éléments d'un tableau de réel */

void
Statistic::logtab(QVector<double> &tab,QVector<double> &tabTemp,int n)
{
    tab_temp.resize(n);   //Réservation de l'espace mémoire

    for (int i=0;i<n;i++)
    {
        tabTemp[i]=(log10(tab[i]));
    }
}

/* Fonction d'inverstion des élements d'un tableau de réel */

void
Statistic::invtab(QVector<double> &tab,QVector<double> &tabTemp,int n)
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

void
Statistic::ecart_a_moyenne(QVector<double> &tab,double Moyenne,int n)
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

double
Statistic::covariance(QVector<double> &Xi, QVector<double> &Yi,int n)
{
    double cov;

    produittab(Xi,Yi,n);
    cov = moyenne(tab_temp,n) - ( moyenne(Xi,n) * moyenne(Yi,n) );

    return (cov);
}

/* Fonction de calcul de la somme des carrés des écarts a la moyenne */

double
Statistic::variance(QVector<double> &val,int n)
{
    double sce;

    produittab(val,val,n);
    sce = moyenne(tab_temp,n) - ( moyenne(val,n) * moyenne(val,n));

    return (sce);
}

/* Fonction de calcul de l'écart-type */

double
Statistic::ecarttype(QVector<double> &val,int n)
{
    double ect= sqrt(variance(val,n));

    return (ect);
}
/******************************************************/
/* Fonctions pour le calcul de la régression linéaire */
/* par la méthode des moindres carré                  */
/******************************************************/

/* Fonction de clacul de la pente (a) */

double
Statistic::pente(QVector<double> &Xi,QVector<double> &Yi,int n)
{
    double a = covariance(Xi,Yi,n)/variance(Xi,n);

    return (a);
}

/* Fonction de clacul de l'ordonnée a l'origine (b) */

double
Statistic::ordonnee(QVector<double> &Xi,QVector<double> &Yi,int n)
{
    double b = moyenne(Yi,n) - ( pente(Xi,Yi,n) * moyenne(Xi,n) );

    return (b);
}

/* Fonction de calcul du coef de corrélation (r) */

double
Statistic::corr(QVector<double> &Xi, QVector<double> &Yi,int n)
{
    double r = covariance(Xi,Yi,n)/(ecarttype(Xi,n)*ecarttype(Yi,n));
        //double r=pente(Xi,Yi,n)*pente(Xi,Yi,n)*(variance(Xi,n)/variance(Yi,n));
    return (r);
}

/* Fonction de détermination du meilleur ajustement */

int
Statistic::ajustement(QVector<double> &Xi,QVector<double> &Yi,int n)
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
