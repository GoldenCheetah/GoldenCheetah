set title "Critical Power Output"
set xlabel "Interval Duration (MM:SS)"
set ylabel "Average Power (Watts)"
set mxtics 5
set mytics 2
set yrange [0:]
set xrange [0.021:120]
set logscale x
set xtics ("0:01.26" 0.021, "0:05" 0.08333, "0:12" 0.2, "1:00" 1, "5:00" 5, \
           "12:00" 12, "30:00" 30, "60:00" 60, "120:00" 120)
plot 'cpint.out' noti with li lt 1
pause -1
