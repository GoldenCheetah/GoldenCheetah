/*
 * Copyright (c) 2018 Mark Liversedge (liversedge@gmail.com)
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


#include "PowerProfile.h"

#include <QFile>
#include <QTextStream>

struct PowerPercentile powerPercentile[]={

// based upon V0.2 of the OpenData Power profile
{ 99.99,593.3736,8.511083789,540.6868,7.28851235,492.9682,6.873482126,459.8246216,6.39594671,436.4760517,6.267013201,39929.81539,661.8325434,2495.3434,39.4455708 },
{ 99,516.48,7.442829332,453.24,6.416262295,438.24,6.136744928,414.4469154,5.8191388,401.9391371,5.671368232,37949.52507,526.9470541,2451.24,34.09693072 },
{ 98,504,7.115806452,440.48,6.199790105,410.24,5.873734266,397.1888316,5.5906764,378.4551804,5.36084711,36252.91351,498.9983697,2395.36,33.06969395 },
{ 95,475.2,6.62504867,407.2,5.804530319,383,5.477230769,367.665045,5.259987,351.772508,4.983962631,32177.61056,442.8424905,2128.3,29.72908576 },
{ 90,446.2,6.284169884,387,5.43251311,363,5.093714286,347.4375,4.883274,332.7666377,4.614217173,28717.99565,393.922918,1932.4,25.92047167 },
{ 85,428.3,5.97338843,369.3,5.153253169,347,4.821903968,335.589169,4.670734,319.7532853,4.437054365,26410.59251,361.8326987,1717.2,23.27294337 },
{ 80,412,5.784285714,356,4.955685624,335,4.680879065,321.606502,4.488434,308.0188973,4.281274492,24281.06186,337.9441078,1489,20.84863362 },
{ 75,399,5.551119403,344,4.834084084,325,4.548088518,313.42792,4.37803,298.845795,4.161000332,22558.67185,316.3123818,1399.5,19.44322992 },
{ 70,387.6,5.401463415,336,4.69396728,318,4.413378026,304.012918,4.232546,289.5113431,4.011858016,21537.10107,294.7501785,1329,18.2034371 },
{ 65,378,5.256818182,330,4.577460467,311,4.305830973,297.52703,4.14874,282.6207701,3.912565095,20453.01805,275.2567854,1266,17.34846154 },
{ 60,372,5.108607079,322,4.455961875,305,4.186619718,292.96867,4.024048,277.1309134,3.814979977,19368.50974,261.6989624,1201,16.43307576 },
{ 55,363,4.96106088,317.9,4.329325735,298,4.071344538,285.933962,3.88735,269.2429466,3.695022723,18423.4887,251.8202718,1160,15.86292898 },
{ 50,354,4.817073171,309,4.207792208,289,3.984375,276.55,3.78647,261.4774677,3.576334131,17279.58259,236.4562234,1120,15.03125 },
{ 45,345,4.702744932,300,4.08385914,282,3.846524785,268.060583,3.693006,254.6373505,3.488324083,16271.61241,224.5534225,1072.08,14.4734879 },
{ 40,335,4.534371513,292,3.986409517,275,3.746516432,262.56617,3.580864,248.752736,3.391197883,15443.15691,211.3098027,1015.2,13.81834851 },
{ 35,325.3,4.376516425,284,3.849315068,266,3.649344054,255.287,3.48246,242.3614973,3.283477464,14439.95701,194.8085742,967.3,13.28068241 },
{ 30,311,4.26407644,274,3.75,258,3.519184613,245.910502,3.374102,233.6858191,3.187564461,13429.78285,176.3658671,927,12.6242049 },
{ 25,301,4.079416623,266,3.619452786,250,3.408604452,239.27083,3.273685,225.986018,3.076581297,12211.28783,163.6486562,867.5,11.79395207 },
{ 20,288.6,3.90972549,256.6,3.462034739,240,3.290151515,229.24517,3.118474,216.91052,2.893825896,10741.54876,146.244684,808.7828,11.00553652 },
{ 15,275.7,3.743016555,245,3.301874235,228.7,3.096475926,218.346829,2.957496,206.7232226,2.766966507,9436.101133,128.6555223,753.4,10.17099286 },
{ 10,257,3.418423665,226.8,3.027478916,213,2.82431746,202.345666,2.697628,191.2953882,2.560877734,8002.893643,108.5086484,667.1616,9.048702554 },
{ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }};

QString
PowerPercentile::rank(type x, double value)
{
    for(int i=0; powerPercentile[i].percentile > 0; i++) {
        if (value > powerPercentile[i].value(x)) {
            return QString("%1%").arg(powerPercentile[i].percentile);
        }
    }
    return "10%";
}

PowerProfile powerProfile, powerProfileWPK;
void initPowerProfile()
{
    // read in data from resources
    int lineno=0;
    QFile pp(":data/powerprofile.csv");
    if (pp.open(QIODevice::ReadOnly)) {

        QTextStream is(&pp);

        while (!is.atEnd()) {

            // readit and setup structures
            QString row=is.readLine();
            lineno++;

            // first line is headers, Percentile followed by 1,2,3 ... 36000
            switch (lineno) {

                case 1:
                {
                    foreach(QString head, row.split(",")) {
                        if (head != "Percentile") {
                            powerProfile.seconds << head.toDouble() / 60.0f; // cpplot wants in minutes
                        }
                    }
                }
                break;

                default:
                {
                    QVector<double> values;
                    QStringList tokens = row.split(",");
                    double percentile = tokens[0].toDouble();
                    powerProfile.percentiles << percentile;
                    for(int i=1; i<tokens.count(); i++) values << tokens[i].toDouble();
                    powerProfile.values.insert(percentile, values);
                }
                break;
            }

        }
        pp.close();
    }
    // read in data from resources
    lineno=0;
    QFile pw(":data/powerprofilewpk.csv");
    if (pw.open(QIODevice::ReadOnly)) {

        QTextStream is(&pw);

        while (!is.atEnd()) {

            // readit and setup structures
            QString row=is.readLine();
            lineno++;

            // first line is headers, Percentile followed by 1,2,3 ... 36000
            switch (lineno) {

                case 1:
                {
                    foreach(QString head, row.split(",")) {
                        if (head != "Percentile") {
                            powerProfileWPK.seconds << head.toDouble() / 60.0f; // cpplot wants in minutes
                        }
                    }
                }
                break;

                default:
                {
                    QVector<double> values;
                    QStringList tokens = row.split(",");
                    double percentile = tokens[0].toDouble();
                    powerProfileWPK.percentiles << percentile;
                    for(int i=1; i<tokens.count(); i++) values << tokens[i].toDouble();
                    powerProfileWPK.values.insert(percentile, values);
                }
                break;
            }

        }
        pw.close();
    }
}
