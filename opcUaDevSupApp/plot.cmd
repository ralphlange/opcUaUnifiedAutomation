set timefmt "%H:%M:%S"
set xdata time

plot 'rdPEW272REAL.csv' using 3:5 title 'dVal/dT', 'rdPEW272REAL.csv' using 3:6 title 'dT'
pause 100
