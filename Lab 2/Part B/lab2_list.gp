#! /usr/bin/gnuplot
#
# purpose:
#     generate data reduction graphs for the multi-threaded list project
#
# input: lab2b_list.csv
#    1. test name
#    2. # threads
#    3. # iterations per thread
#    4. # lists
#    5. # operations performed (threads x iterations x (ins + lookup + delete))
#    6. run time (ns)
#    7. run time per operation (ns)
#    8. lock time per operation (ns)
#
# output:
#    lab2b_1.png ... operations per second vs number of threads for mutex and spin-lock options
#    lab2b_2.png ... average operation time vs lock wait time of mutex option
#    lab2b_3.png ... iterations that run successfully vs threads with --lists=4 option
#    lab2b_4.png ... throughput vs threads with mutex lock
#    lab2b_5.png ... throughput vs threads with spin-lock
#
# Note:
#    Managing data is simplified by keeping all of the results in a single
#    file.  But this means that the individual graphing commands have to
#    grep to select only the data they want.
#
#    Early in your implementation, you will not have data for all of the
#    tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "List-1: Throughput of Mutex and Spin-lock Options"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_1.png'
set key right top
plot \
"< grep -e 'list-none-m,[1,2]*[0-9],1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'list w/mutex' with linespoints lc rgb 'blue', \
"< grep -e 'list-none-s,[1,2]*[0-9],1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'list w/spin-lock' with linespoints lc rgb 'green'

set title "List-2: Average Operation Time and Lock Wait Time of Mutex"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Average Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
set key right top
plot \
"< grep -e 'list-none-m,[1,2]*[0-9],1000,1,' lab2b_list.csv" using ($2):($8) \
title 'Average Lock Wait Time' with linespoints lc rgb 'blue', \
"< grep -e 'list-none-m,[1,2]*[0-9],1000,1,' lab2b_list.csv" using ($2):($7) \
title 'Average Operation Time' with linespoints lc rgb 'green'

set title "List-3: Iterations that run successfully and Threads with lists=4"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Iterations that run without failure"
set logscale y 10
set output 'lab2b_3.png'
set key right top
plot \
"< grep -E 'list-id-none,1*[0-9],1*[0-9],4,' lab2b_list.csv" using ($2):($3) \
title 'No synchronization' with points lc rgb 'blue', \
"< grep -E 'list-id-m,1*[0-9],[1-8]0,4,' lab2b_list.csv" using ($2):($3) \
title 'Mutex lock' with points lc rgb 'green', \
"< grep -E 'list-id-s,1*[0-9],[1-8]0,4,' lab2b_list.csv" using ($2):($3) \
title 'Spin-lock' with points lc rgb 'red'

set title "List-4: Throughput and threads with mutex lock"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_4.png'
set key right top
plot \
"< grep -E 'list-none-m,1*[0-9],1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=1' with linespoints lc rgb 'blue', \
"< grep -E 'list-none-m,1*[0-9],1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=4' with linespoints lc rgb 'green', \
"< grep -E 'list-none-m,1*[0-9],1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=8' with linespoints lc rgb 'red', \
"< grep -E 'list-none-m,1*[0-9],1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=16' with linespoints lc rgb 'orange'

set title "List-5: Throughput and threads with spin-lock"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set output 'lab2b_5.png'
set key right top
plot \
"< grep -E 'list-none-s,1*[0-9],1000,1,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=1' with linespoints lc rgb 'blue', \
"< grep -E 'list-none-s,1*[0-9],1000,4,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=4' with linespoints lc rgb 'green', \
"< grep -E 'list-none-s,1*[0-9],1000,8,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=8' with linespoints lc rgb 'red', \
"< grep -E 'list-none-s,1*[0-9],1000,16,' lab2b_list.csv" using ($2):(1000000000/($7)) \
title 'lists=16' with linespoints lc rgb 'orange'