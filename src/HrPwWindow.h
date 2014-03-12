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

#ifndef _GC_HrPwWindow_h
#define _GC_HrPwWindow_h 1

#include <QWidget>
#include <QFormLayout>

#include "GoldenCheetah.h"
#include "LTMWindow.h" // for tooltip/canvaspicker

class QCheckBox;
class QLineEdit;
class RideItem;
class HrPwPlot;
class Context;
class SmallPlot;
class QSlider;



class HrPwWindow : public GcChartWindow
{
    Q_OBJECT
    G_OBJECT

    Q_PROPERTY(int shadeZones READ isShadeZones WRITE setShadeZones USER true)
    Q_PROPERTY(int joinLine READ isJoinLine WRITE setJoinLine USER true)
    Q_PROPERTY(int fullplot READ showFullplot WRITE setFullplot USER true)

    int isJoinLine() const { return joinlineCheckBox->checkState(); }
    void setJoinLine(int x) { joinlineCheckBox->setCheckState(x ? Qt::Checked : Qt::Unchecked); }
    int isShadeZones() const { return shadeZones->checkState(); }
    void setShadeZones(int x) { shadeZones->setCheckState(x ? Qt::Checked : Qt::Unchecked); }
    int showFullplot() const { return fullplot->checkState(); }
    void setFullplot(int x) { fullplot->setCheckState(x ? Qt::Checked : Qt::Unchecked); }

    public:

        HrPwWindow(Context *context);
        void setData(RideItem *item);
        int findDelay(QVector<double> &wattsArray, QVector<double> &hrArray, int rideTimeSecs);

        // Maths functions used by HrPwPlot
        double pente(QVector<double> &Xi,QVector<double> &Yi,int n);
        double ordonnee(QVector<double> &Xi,QVector<double> &Yi,int n);
        double corr(QVector<double> &Xi, QVector<double> &Yi,int n);
        double moyenne(QVector<double> &Xi,int n);

        // reveal
        bool hasReveal() { return true; }

        int smooth;

    public slots:
        void rideSelected();
        void configChanged();

    protected slots:
        void setJoinLineFromCheckBox();
        void setDelayFromLineEdit();
        void setDelayFromSlider();
        void setSmoothingFromLineEdit();
        void setSmoothingFromSlider();
        void setrDelayFromLineEdit();
        void setrDelayFromSlider();
        void setrSmoothingFromLineEdit();
        void setrSmoothingFromSlider();
        void setShadeZones();
        void setSmooth(int);
        void showHidePlot();
        void setDelay(int);

    protected:
        Context *context;
        HrPwPlot  *hrPwPlot;
        SmallPlot *smallPlot;

        RideItem *current;

        QCheckBox *joinlineCheckBox;
        QCheckBox *shadeZones;
        QCheckBox *fullplot;

        QSlider *delaySlider;
        QLineEdit *delayEdit;

        QSlider *smoothSlider;
        QLineEdit *smoothEdit;

    private:
        // reveal controls
        QLabel *rSmooth;
        QSlider *rSmoothSlider;
        QLineEdit *rSmoothEdit;
        QLabel *rDelay;
        QSlider *rDelaySlider;
        QLineEdit *rDelayEdit;

        // Maths functions used by the plots
        QVector<double> tab_temp; //Déclaration d'un tableau temporaire
        int test_zero(QVector<double> &tab,int n);
        int test_negatif(QVector<double> &tab,int n);
        void logtab(QVector<double> &tab,QVector<double> &tabTemp,int n);
        void lntab(QVector<double> &tab,QVector<double> &tabTemp,int n);
        void invtab(QVector<double> &tab,QVector<double> &tabTemp,int n);
        int ajustement(QVector<double> &Xi,QVector<double> &Yi,int n);
        double moyenne(QVector<int> &tab,int n);
        double moyenne2(double somme,int n);
        double val_abs(double x);
        int rmax(QVector<double> &r);
        int somme(QVector<int> &tab,int n);
        double somme(QVector<double> &tab,int n);
        void produittab(QVector<double> &tab1,QVector<double> &tab2,int n);
        void ecart_a_moyenne(QVector<double> &tab,double Moyenne,int n);
        double covariance(QVector<double> &Xi, QVector<double> &Yi,int n);
        double variance(QVector<double> &val,int n);
        double ecarttype(QVector<double> &val,int n);
};

#endif // _GC_HrPwWindow_h
