set title "May 3, 2006 16:24:04"
set xlabel "Ride Time (minutes)"
plot \
     "< awk '/^[^#]/ {print $1, $4}' 2006_05_03_16_24_04.dat | ./smooth.pl 10" ti 'Watts' wi li, \
     "< awk '/^[^#]/ {print $1, $7}' 2006_05_03_16_24_04.dat | ./smooth.pl 10" ti 'Heartrate' wi li, \
     "< awk '/^[^#]/ {print $1, $6}' 2006_05_03_16_24_04.dat | ./smooth.pl 10" ti 'Cadence' wi li, \
     "< awk '/^[^#]/ {print $1, $3}' 2006_05_03_16_24_04.dat | ./smooth.pl 10" ti 'MPH' wi li
pause -1
