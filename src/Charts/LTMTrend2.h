/*
 * Copyright (c) 2010 Mark Liversedge (liversedge@gmail.com)
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

#ifndef _GC_LTMTrend2_h
#define _GC_LTMTrend2_h

class LTMTrend2
{
    public:

        double minx, miny, maxx, maxy;

        // constructor just initialises variables
        LTMTrend2(const double *xdata, const double *ydata, int count)
                : minx(10000), miny(10000), maxx(-10000), maxy(-10000), count(count),
                  sx4(0), sx3(0), sx2(0), sx(0), sx2y(0), sxy(0), sy(0)
        {
            for (int i = 0; i < count; i++) {
                addPoint(xdata[i], ydata[i]);
            }
        }

        void addPoint(double x, double y) 
        {
            if (x > maxx) maxx = x;
            if (x < minx) minx = x;
            if (y > maxy) maxy = y;
            if (y < miny) miny = y;

            // now sum the squares etc we use to determine a b and c
            sx += x;
            sy += y;
            sxy += x*y;

            sx4 += pow(x, 4);
            sx3 += pow(x, 3);
            sx2 += pow(x, 2);

            sx2y += pow(x,2) * y;
        }

        /// returns the a term of the equation ax^2 + bx + c
        double a()
        {
            if (count < 3) return 0;
            
            return (sx2y*(sx2 * count - sx * sx) - sxy*(sx3 * count - sx * sx2) + sy*(sx3 * sx - sx2 * sx2)) / (sx4*(sx2 * count - sx * sx) - sx3*(sx3 * count - sx * sx2) + sx2*(sx3 * sx - sx2 * sx2));

        }

        /// returns the b term of the equation ax^2 + bx + c
        double b()
        {
            if (count < 3) return 0;
        
            return (sx4*(sxy * count - sy * sx) - sx3*(sx2y * count - sy * sx2) + sx2*(sx2y * sx - sxy * sx2)) / (sx4 * (sx2 * count - sx * sx) - sx3 * (sx3 * count - sx * sx2) + sx2 * (sx3 * sx - sx2 * sx2));

        }

        /// returns the c term of the equation ax^2 + bx + c
        double c()
        {
            if (count < 3) return 0;
        
            return (sx4*(sx2 * sy - sx * sxy) - sx3*(sx3 * sy - sx * sx2y) + sx2*(sx3 * sxy - sx2 * sx2y)) / (sx4 * (sx2 * count - sx * sx) - sx3 * (sx3 * count - sx * sx2) + sx2 * (sx3 * sx - sx2 * sx2));

        }
        double yForX(double x)
        {
            //returns value of y predicted by the equation for a given value of x
            return a() * pow(x, 2) + b() * x + c();
        }
#if 0 
        double rSquared() // get r-squared
        {
            if (count < 3) return 0;
            return 1 - getSSerr() / getSStot();
        }
    
#endif

   protected:
#if 0
    private double getSStot() // total sum of squares
    {
        //the sum of the squares of the differences between 
        //the measured y values and the mean y value
        double ss_tot = 0;
        foreach (double[] ppair in pointArray)
        {
            ss_tot += Math.Pow(ppair[1] - (sy / count), 2);
        }
        return ss_tot;
    }

    private double getSSerr() // residual sum of squares
    {
        //the sum of the squares of te difference between 
        //the measured y values and the values of y predicted by the equation
        double ss_err = 0;
        foreach (double[] ppair in pointArray)
        {
            ss_err += Math.Pow(ppair[1] - getPredictedY(ppair[0]), 2);
        }
        return ss_err;
    }
#endif

    /* calculated as points added then used to derive a b and c */
    int count;
    double sx4, sx3, sx2, sx; // sum x^4, ^3, ^2 and just sum x
    double sx2y, sxy, sy;     // sum x^2 * y, sum x * y, and just sum y


};

#endif
